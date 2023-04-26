# ButtonBoard2023

You'll want to install the [Pico-W-Go extension](https://marketplace.visualstudio.com/items?itemName=paulober.pico-w-go) and its requisites for this to be convenient.

This uses a design from [Adafruit](https://learn.adafruit.com/diy-pico-mechanical-keyboard-with-fritzing-circuitpython) to make a 21-key operator input board using a Raspberry Pi Pico.

The source code has been modified to send HID Gamepad commands instead of keyboard input, so that FRC Driver Station can recognize it as a controller. The Java code that runs on the robot has a utility class that uses the GenericHID classes provided by WPILib -- right now, it's in [frc5431/RobotCode2023](https://github.com/frc5431/RobotCode2023/tree/master/src/main/java/frc/robot/util).