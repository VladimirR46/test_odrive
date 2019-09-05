#ifndef __ENCODER_HPP
#define __ENCODER_HPP

#ifndef __ODRIVE_MAIN_H
#error "This file should not be included directly. Include odrive_main.h instead."
#endif

class Encoder {
public:
    enum Error_t {
        ERROR_NONE = 0,
        ERROR_UNSTABLE_GAIN = 0x01,
        ERROR_CPR_OUT_OF_RANGE = 0x02,
        ERROR_NO_RESPONSE = 0x04,
        ERROR_UNSUPPORTED_ENCODER_MODE = 0x08,
        ERROR_ILLEGAL_HALL_STATE = 0x10,
        ERROR_INDEX_NOT_FOUND_YET = 0x20,
        ERROR_ABS_SPI_TIMEOUT = 0x40,
        ERROR_ABS_SPI_COM_FAIL = 0x80,
        ERROR_ABS_SPI_NOT_READY = 0x100,
    };

    enum Mode_t {
        MODE_INCREMENTAL,
        MODE_HALL,
        MODE_SPI_ABS_CUI = 0x100,
        MODE_SPI_ABS_AMS = 0x101,
    };
    const uint32_t MODE_FLAG_ABS = 0x100;

    struct Config_t {
        Encoder::Mode_t mode = Encoder::MODE_INCREMENTAL;
        bool use_index = false;
        bool pre_calibrated = false; // If true, this means the offset stored in
                                    // configuration is valid and does not need
                                    // be determined by run_offset_calibration.
                                    // In this case the encoder will enter ready
                                    // state as soon as the index is found.
        float idx_search_speed = 10.0f; // [rad/s electrical]
        bool zero_count_on_find_idx = true;
        int32_t cpr = (2048 * 4);   // Default resolution of CUI-AMT102 encoder,
        int32_t offset = 0;        // Offset between encoder count and rotor electrical phase
        float offset_float = 0.0f; // Sub-count phase alignment offset
        float calib_range = 0.02f;
        float bandwidth = 2000.0f;
        bool ignore_illegal_hall_state = false;
        uint16_t abs_spi_cs_gpio_pin = 0;
    };

    Encoder(const EncoderHardwareConfig_t& hw_config,
                     Config_t& config);
    
    void setup();
    void set_error(Error_t error);
    bool do_checks();

    void enc_index_cb();

    void set_linear_count(int32_t count);
    void set_circular_count(int32_t count, bool update_offset);
    bool calib_enc_offset(float voltage_magnitude);
    bool scan_for_enc_idx(float omega, float voltage_magnitude);

    bool run_index_search();
    bool run_offset_calibration();
    bool update();

    void update_pll_gains();

    const EncoderHardwareConfig_t& hw_config_;
    Config_t& config_;
    Axis* axis_ = nullptr; // set by Axis constructor

    Error_t error_ = ERROR_NONE;
    bool index_found_ = false;
    bool is_ready_ = false;
    int32_t shadow_count_ = 0;
    int32_t count_in_cpr_ = 0;
    float interpolation_ = 0.0f;
    float phase_ = 0.0f;    // [count]
    float pos_estimate_ = 0.0f;  // [count]
    float pos_cpr_ = 0.0f;  // [count]
    float vel_estimate_ = 0.0f;  // [count/s]
    float pll_kp_ = 0.0f;   // [count/s / count]
    float pll_ki_ = 0.0f;   // [(count/s^2) / count]
    int32_t pos_abs_ = 0;
    float spi_error_rate_ = 0.0f;
    float pos_abs_filter_ = 0.0f;
    float lp_filter_coefficient_ = 0.01f;

    // Updated by low_level pwm_adc_cb
    uint8_t hall_state_ = 0x0; // bit[0] = HallA, .., bit[2] = HallC

    bool abs_spi_init();
    bool abs_spi_start_transaction();
    void abs_spi_cb();
    void abs_spi_cs_pin_init();
    uint16_t abs_spi_dma_tx_[2] = {0xFFFF, 0x0000};
    uint16_t abs_spi_dma_rx_[2];
    bool abs_spi_pos_updated_;
    GPIO_TypeDef* abs_spi_cs_port_;
    uint16_t abs_spi_cs_pin_;
    uint32_t abs_spi_cr1;
    uint32_t abs_spi_cr2;

    // Communication protocol definitions
    auto make_protocol_definitions() {
        return make_protocol_member_list(
            make_protocol_property("error", &error_),
            make_protocol_ro_property("is_ready", &is_ready_),
            make_protocol_ro_property("index_found", const_cast<bool*>(&index_found_)),
            make_protocol_property("shadow_count", &shadow_count_),
            make_protocol_property("count_in_cpr", &count_in_cpr_),
            make_protocol_property("interpolation", &interpolation_),
            make_protocol_property("phase", &phase_),
            make_protocol_property("pos_estimate", &pos_estimate_),
            make_protocol_property("pos_cpr", &pos_cpr_),
            make_protocol_property("hall_state", &hall_state_),
            make_protocol_property("vel_estimate", &vel_estimate_),
            make_protocol_property("pos_abs", &pos_abs_),
            make_protocol_property("pos_abs_filter", &pos_abs_filter_),
            make_protocol_property("lp_filter_coefficient", &lp_filter_coefficient_),
            // make_protocol_property("pll_kp", &pll_kp_),
            // make_protocol_property("pll_ki", &pll_ki_),
            make_protocol_object("config",
                make_protocol_property("mode", &config_.mode,
                    [](void* ctx) { static_cast<Encoder*>(ctx)->abs_spi_init(); }, this),
                make_protocol_property("use_index", &config_.use_index),
                make_protocol_property("abs_spi_cs_gpio_pin", &config_.abs_spi_cs_gpio_pin,
                    [](void* ctx) { static_cast<Encoder*>(ctx)->abs_spi_cs_pin_init(); }, this),
                make_protocol_property("pre_calibrated", &config_.pre_calibrated),
                make_protocol_property("idx_search_speed", &config_.idx_search_speed),
                make_protocol_property("zero_count_on_find_idx", &config_.zero_count_on_find_idx),
                make_protocol_property("cpr", &config_.cpr),
                make_protocol_property("offset", &config_.offset),
                make_protocol_property("offset_float", &config_.offset_float),
                make_protocol_property("bandwidth", &config_.bandwidth,
                    [](void* ctx) { static_cast<Encoder*>(ctx)->update_pll_gains(); }, this),
                make_protocol_property("calib_range", &config_.calib_range),
                make_protocol_property("ignore_illegal_hall_state", &config_.ignore_illegal_hall_state)
            )
        );
    }
};

DEFINE_ENUM_FLAG_OPERATORS(Encoder::Error_t)

#endif // __ENCODER_HPP
