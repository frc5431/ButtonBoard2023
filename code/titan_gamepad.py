import struct
import time

import usb_hid  # type: ignore


def find_device(
    devices: list[usb_hid.Device], *, usage_page: int, usage: int
) -> usb_hid.Device:
    """Search through the provided sequence of devices to find the one with the matching
    usage_page and usage."""
    if hasattr(devices, "send_report"):
        devices = [devices]  # type: ignore
    for device in devices:
        if (
            device.usage_page == usage_page  # type: ignore
            and device.usage == usage  # type: ignore
            and hasattr(device, "send_report")
        ):
            return device
    raise ValueError("Could not find matching HID device.")


class Gamepad:
    """Emulate a generic gamepad controller with 32 buttons,
    numbered 1-32."""

    def __init__(self, devices):
        """Create a Gamepad object that will send USB gamepad HID reports.

        Devices can be a list of devices that includes a gamepad device or a gamepad device
        itself. A device is any object that implements ``send_report()``, ``usage_page`` and
        ``usage``.
        """
        self._gamepad_device = find_device(devices, usage_page=0x01, usage=0x05)

        # Reuse this bytearray to send mouse reports.
        # Typically controllers start numbering buttons at 1 rather than 0.
        # report[0] buttons 1-8 (LSB is button 1)
        # report[1] buttons 9-16
        # report[2] buttons 17-24
        # report[3] buttons 25-32 (MSB is button 32)
        self._report = bytearray(4)

        # Remember the last report as well, so we can avoid sending
        # duplicate reports.
        self._last_report = bytearray(4)

        # Store settings separately before putting into report. Saves code
        # especially for buttons.
        self._buttons_state = 0

        # Send an initial report to test if HID device is ready.\
        # If not, wait a bit and try once more.
        try:
            self.reset_all()
        except OSError:
            time.sleep(1)
            self.reset_all()

    def press_buttons(self, *buttons):
        """Press and hold the given buttons."""
        for button in buttons:
            self._buttons_state |= 1 << self._validate_button_number(button) - 1
        self._send()

    def release_buttons(self, *buttons):
        """Release the given buttons."""
        for button in buttons:
            self._buttons_state &= ~(
                1 << self._validate_button_number(button) - 1
            )
        self._send()

    def release_all_buttons(self):
        """Release all the buttons."""
        self._buttons_state = 0
        self._send()

    def click_buttons(self, *buttons):
        """Press and release the given buttons."""
        self.press_buttons(*buttons)
        self.release_buttons(*buttons)

    # def move_joysticks(self, x=None, y=None, z=None, r_z=None):
    #     """Set and send the given joystick values.
    #     The joysticks will remain set with the given values until changed

    #     One joystick provides ``x`` and ``y`` values,
    #     and the other provides ``z`` and ``r_z`` (z rotation).
    #     Any values left as ``None`` will not be changed.

    #     All values must be in the range -127 to 127 inclusive.

    #     Examples::

    #         # Change x and y values only.
    #         gp.move_joysticks(x=100, y=-50)

    #         # Reset all joystick values to center position.
    #         gp.move_joysticks(0, 0, 0, 0)
    #     """
    #     if x is not None:
    #         self._joy_x = self._validate_joystick_value(x)
    #     if y is not None:
    #         self._joy_y = self._validate_joystick_value(y)
    #     if z is not None:
    #         self._joy_z = self._validate_joystick_value(z)
    #     if r_z is not None:
    #         self._joy_r_z = self._validate_joystick_value(r_z)
    #     self._send()

    def reset_all(self):
        """Release all buttons and set joysticks to zero."""
        self._buttons_state = 0
        self._send(always=True)

    def _send(self, always=False):
        """Send a report with all the existing settings.
        If ``always`` is ``False`` (the default), send only if there have been changes.
        """
        struct.pack_into("@l", self._report, 0, self._buttons_state)

        if always or self._last_report != self._report:
            self._gamepad_device.send_report(self._report)
            # Remember what we sent, without allocating new storage.
            self._last_report[:] = self._report

    @staticmethod
    def _validate_button_number(button):
        if not 1 <= button <= 32:
            raise ValueError("Button number must in range 1 to 32")
        return button

    # @staticmethod
    # def _validate_joystick_value(value):
    #     if not -127 <= value <= 127:
    #         raise ValueError("Joystick value must be in range -127 to 127")
    #     return value
