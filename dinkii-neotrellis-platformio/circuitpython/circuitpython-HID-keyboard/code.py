# create the i2c object for the trellis
#i2c_bus = board.I2C()  # uses board.SCL and board.SDA
#i2c_bus = board.STEMMA_I2C()  # For using the built-in STEMMA QT connector on a microcontroller
#i2c = busio.I2C(board.SCL1, board.SDA1)


# SPDX-FileCopyrightText: 2022 ladyada for Adafruit Industries
# SPDX-License-Identifier: MIT

import time
import board
import busio
from adafruit_neotrellis.neotrellis import NeoTrellis
import rainbowio

import usb_hid
from adafruit_hid.keyboard import Keyboard
from adafruit_hid.keyboard_layout_us import KeyboardLayoutUS
from adafruit_hid.keycode import Keycode

# create the i2c object for the trellis
i2c_bus = board.I2C()  # uses board.SCL and board.SDA
# i2c_bus = busio.I2C(board.SCL1, board.SDA1)

keyboard = Keyboard(usb_hid.devices)
layout = KeyboardLayoutUS(keyboard)

# create the trellis
trellis = NeoTrellis(i2c_bus)

# holds whether pad is currently lit or not, when it was pressed before being ack'd
# tuple of (pad lit?, pad_press_time_or_zero_if_ackd)
pad_states = [(False,0)] * 64

# Set the brightness value (0 to 1.0)
trellis.brightness = 0.5

# some color definitions
OFF = (0, 0, 0)
RED = (255, 0, 0)
YELLOW = (255, 150, 0)
GREEN = (0, 255, 0)
CYAN = (0, 255, 255)
BLUE = (0, 0, 255)
PURPLE = (180, 0, 255)
WHITE = (100, 100, 100)

YELLOW_GREEN = (127, 255, 0)
LIGHT_BLUE = (0, 127, 255)
ORANGE = (255, 80, 0)
PINK = (255, 0, 255)
ROUGE = (255, 0, 127)

WHITE_WARM = (120, 100, 80)
WHITE_COOL = (80, 100, 120)
WHITE_GREEN = (80, 120, 100)


colors = [OFF, RED, YELLOW, GREEN, CYAN, BLUE, PURPLE, 
YELLOW_GREEN, LIGHT_BLUE, ORANGE, PINK, ROUGE, 
WHITE, WHITE_WARM, WHITE_COOL, WHITE_GREEN]

color_table = [  # you can make custom color sections for clarity
  1, 2, 3, 4, 5, 6, 1, 2,
  3, 4, 5, 6, 1, 2, 3, 4
]

def dimmed_colors(color_values):
    (red_value, green_value, blue_value) = color_values
    return (red_value // 10, green_value // 10, blue_value // 10)
    
num_switches = 16

# A map of keycodes that will be mapped sequentially to each of the keys, 0-15
keymap =    [Keycode.ZERO,
             Keycode.ONE,
             Keycode.TWO,
             Keycode.THREE,
             Keycode.FOUR,
             Keycode.FIVE,
             Keycode.SIX,
             Keycode.SEVEN,
             Keycode.EIGHT,
             Keycode.NINE,
             Keycode.A,
             Keycode.B,
             Keycode.C,
             Keycode.D,
             Keycode.E,
             Keycode.F]


def playback_led_colors():
    for i in range(num_switches):
        trellis.pixels[i] = colors[color_table[i]]


playback_led_colors()

# convert an x,y (0-7,0-7) to 0-63
def xy_to_pos(x,y):
    return x+(y*8)

# convert 0-63 to x,y
def pos_to_xy(pos):
    return (pos%8, pos//8)

# callback when pads are pressed
def handle_pad(event):
    pos = event.number
    

    if event.edge == NeoTrellis.EDGE_RISING:
        trellis.pixels[event.number] = colors[color_table[event.number]]

        # HID stuff
        keycode = keymap[pos]
        keyboard.send(keycode)
        
    elif event.edge == NeoTrellis.EDGE_FALLING:
        trellis.pixels[event.number] = OFF



# this will be called when button events are received
def blink(event):
    # turn the LED on when a rising edge is detected
    if event.edge == NeoTrellis.EDGE_RISING:
        trellis.pixels[event.number] = CYAN
    # turn the LED off when a falling edge is detected
    elif event.edge == NeoTrellis.EDGE_FALLING:
        trellis.pixels[event.number] = OFF


for i in range(16):
    # activate rising edge events on all keys
    trellis.activate_key(i, NeoTrellis.EDGE_RISING)

    # activate falling edge events on all keys
    trellis.activate_key(i, NeoTrellis.EDGE_FALLING)

    # set all keys to trigger the callback function
    trellis.callbacks[i] = handle_pad
#     trellis.set_callback(x_pad, y_pad, handle_pad)

    # cycle the LEDs on startup
#     trellis.pixels[i] = PURPLE
    time.sleep(0.05)

for i in range(16):
    trellis.pixels[i] = OFF
    time.sleep(0.05)

while True:
    # call the sync function call any triggered callbacks
    trellis.sync()
    # the trellis can only be read every 17 millisecons or so
    time.sleep(0.02)
