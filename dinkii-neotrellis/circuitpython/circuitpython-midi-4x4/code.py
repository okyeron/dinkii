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
from adafruit_neotrellis.multitrellis import MultiTrellis
import rainbowio
import usb_midi
import adafruit_midi
from adafruit_midi.note_on import NoteOn
from adafruit_midi.note_off import NoteOff

note_base = 36
note_vel = 127
pad_midi_channel = 0  # add one for "real world" MIDI channel number, e.g. 0=1
led_midi_channel = 2  # see ^

num_switches = 64

# holds whether pad is currently lit or not, when it was pressed before being ack'd
# tuple of (pad lit?, pad_press_time_or_zero_if_ackd)
pad_states = [(False,0)] * 64


midi_usb = adafruit_midi.MIDI( midi_in=usb_midi.ports[0],
                               midi_out=usb_midi.ports[1] )

# create the i2c object for the trellis
# i2c_bus = board.I2C()  # uses board.SCL and board.SDA
i2c = busio.I2C(board.SCL, board.SDA)
# i2c_bus = busio.I2C(board.SCL1, board.SDA1)
# i2c_bus = board.STEMMA_I2C()  # For using the built-in STEMMA QT connector on a microcontroller

# create the trellis
# trellis = NeoTrellis(i2c_bus)
trelli = [  # adjust these to match your jumper settings if needed
     [NeoTrellis(i2c, False, addr=0x2F), NeoTrellis(i2c, False, addr=0x2E)],
     [NeoTrellis(i2c, False, addr=0x3E), NeoTrellis(i2c, False, addr=0x36)]
]
trellis = MultiTrellis(trelli)

# Set the brightness value (0 to 1.0)
trellis.brightness = 0.3

# some color definitions
OFF = (0, 0, 0)
RED = (255, 0, 0)
YELLOW = (255, 150, 0)
GREEN = (0, 255, 0)
CYAN = (0, 255, 255)
BLUE = (0, 0, 255)
PURPLE = (180, 0, 255)

colors = [OFF, RED, YELLOW, GREEN, CYAN, BLUE, PURPLE]
color_table = [  # you can make custom color sections for clarity
  1, 1, 1, 1, 5, 5, 5, 5,
  1, 1, 1, 1, 5, 5, 5, 5,
  1, 1, 1, 1, 5, 5, 5, 5,
  1, 1, 1, 1, 5, 5, 5, 5,
  4, 4, 4, 4, 6, 6, 6, 6,
  4, 4, 4, 4, 6, 6, 6, 6,
  4, 4, 4, 4, 6, 6, 6, 6,
  4, 4, 4, 4, 6, 6, 6, 6
]


# convert an x,y (0-7,0-7) to 0-63
def xy_to_pos(x,y):
    return x+(y*8)

# convert 0-63 to x,y
def pos_to_xy(pos):
    return (pos%8, pos//8)


def playback_led_colors():
    for i in range(num_switches):
        xy_pos = pos_to_xy(i)
        trellis.color(xy_pos[0],xy_pos[1], colors[color_table[i]])

playback_led_colors()

# callback when pads are pressed
def handle_pad(x, y, edge):
    pos = xy_to_pos(x,y)
    
    note_val = pos + note_base
    if edge == NeoTrellis.EDGE_RISING:
        xy_pos = pos_to_xy(pos)
        trellis.color(xy_pos[0],xy_pos[1], colors[color_table[pos]])
        noteon = NoteOn(note_val, note_vel, channel=pad_midi_channel)
        midi_usb.send(noteon)
        
    elif edge == NeoTrellis.EDGE_FALLING:
        xy_pos = pos_to_xy(pos)
        trellis.color(xy_pos[0],xy_pos[1], OFF)
        noteoff = NoteOff(note_val, note_vel, channel=pad_midi_channel)
        midi_usb.send(noteoff)





# this will be called when button events are received
def blink(event):
    # turn the LED on when a rising edge is detected
    if event.edge == NeoTrellis.EDGE_RISING:
        xy_pos = pos_to_xy(event.number)
        trellis.color(xy_pos[0],xy_pos[1], CYAN)
#         trellis.pixels[event.number] = CYAN
    # turn the LED off when a falling edge is detected
    elif event.edge == NeoTrellis.EDGE_FALLING:
        xy_pos = pos_to_xy(event.number)
        trellis.color(xy_pos[0],xy_pos[1], OFF)
#         trellis.pixels[event.number] = OFF


for i in range(num_switches):
    xy_pos = pos_to_xy(i)
    # activate rising edge events on all keys
    trellis.activate_key(xy_pos[0],xy_pos[1], NeoTrellis.EDGE_RISING)
    # activate falling edge events on all keys
    trellis.activate_key(xy_pos[0],xy_pos[1], NeoTrellis.EDGE_FALLING)
    # set all keys to trigger the blink callback
    trellis.set_callback(xy_pos[0], xy_pos[1], handle_pad)

    # cycle the LEDs on startup
#     trellis.pixels[i] = PURPLE
    time.sleep(0.02)

for i in range(num_switches):
    xy_pos = pos_to_xy(i)
    trellis.color(xy_pos[0],xy_pos[1], OFF)
    time.sleep(0.02)

while True:
    # call the sync function call any triggered callbacks
    trellis.sync()
    # the trellis can only be read every 17 millisecons or so
    time.sleep(0.02)
