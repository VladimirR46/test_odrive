#MyODriveControl

#!/usr/bin/env python3
"""
Example usage of the ODrive python library to monitor and control ODrive devices
"""

from __future__ import print_function

import odrive
from odrive.enums import *
import time
import math


print("finding an odrive...")
my_drive = odrive.find_any()

my_drive.axis0.controller.config.control_mode = CTRL_MODE_CURRENT_CONTROL
my_drive.axis0.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
my_drive.axis0.controller.current_setpoint = 0


while True:
    print(-my_drive.axis0.motor.current_control.Iq_measured)
    #time.sleep(0.1)
