import time
import titan_gamepad
import usb_hid  # type: ignore
import board  # type: ignore
import digitalio  # type: ignore

buttons = [
    digitalio.DigitalInOut(pin)
    for pin in (
        board.GP0,
        board.GP1,
        board.GP2,
        board.GP3,
        board.GP4,
        board.GP5,
        board.GP6,
        board.GP7,
        board.GP8,
        board.GP9,
        board.GP10,
        board.GP11,
        board.GP12,
        board.GP13,
        board.GP14,
        board.GP16,
        board.GP17,
        board.GP18,
        board.GP19,
        board.GP20,
        board.GP21,
    )
]


for button in buttons:
    button.direction = digitalio.Direction.INPUT
    button.pull = digitalio.Pull.UP


def getButton(i):
    return not buttons[i].value


def print_debug(states):
    output = "\n"
    # Best fucking line of code I've ever written ^

    for i in range(21):
        output += "🤯" if states[i] else "🏁"
        if i % 7 == 6:
            output += " "

    print(output)


gamepad = titan_gamepad.Gamepad(usb_hid.devices)


def update_hid(states):
    for i in range(21):
        if states[i]:
            gamepad.press_buttons(i + 1)
        else:
            gamepad.release_buttons(i + 1)


while True:
    states = [getButton(i) for i in range(21)]

    print_debug(states)

    update_hid(states)

    # The delay is
    time.sleep(0.01)
