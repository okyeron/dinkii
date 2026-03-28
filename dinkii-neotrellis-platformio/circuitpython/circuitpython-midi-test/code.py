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
import usb_midi
import adafruit_midi
from adafruit_midi.note_on import NoteOn
from adafruit_midi.note_off import NoteOff

note_base = 36
note_vel = 127
pad_midi_channel = 0  # add one for "real world" MIDI channel number, e.g. 0=1
led_midi_channel = 2  # see ^

# holds whether pad is currently lit or not, when it was pressed before being ack'd
# tuple of (pad lit?, pad_press_time_or_zero_if_ackd)
pad_states = [(False,0)] * 64


midi_usb = adafruit_midi.MIDI( midi_in=usb_midi.ports[0],
                               midi_out=usb_midi.ports[1] )

# create the i2c object for the trellis
i2c_bus = board.I2C()  # uses board.SCL and board.SDA
# i2c_bus = busio.I2C(board.SCL1, board.SDA1)
# i2c_bus = board.STEMMA_I2C()  # For using the built-in STEMMA QT connector on a microcontroller

# create the trellis
trellis = NeoTrellis(i2c_bus)

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

colors = [OFF, RED, YELLOW, GREEN, CYAN, BLUE, PURPLE]
color_table = [  # you can make custom color sections for clarity
  1, 2, 3, 4, 5, 6, 1, 2,
  3, 4, 5, 6, 1, 2, 3, 4
]
num_switches = 16

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
    
    note_val = pos + note_base
    if event.edge == NeoTrellis.EDGE_RISING:
#         (pad_on, pad_time) = pad_states[pos] # get pad state & press time
#         print(pad_time)
#         pad_on = not pad_on                  # toggle state
#         pad_states[pos] = (pad_on, time.monotonic()) # and save it w/ new press time
#         print("handle_pad: ", pos, note_val, pad_on)
#         trellis.pixels[event.number] = CYAN
        trellis.pixels[event.number] = colors[color_table[event.number]]
        noteon = NoteOn(note_val, note_vel, channel=pad_midi_channel)
        midi_usb.send(noteon)
        
    elif event.edge == NeoTrellis.EDGE_FALLING:
        trellis.pixels[event.number] = OFF
        noteoff = NoteOff(note_val, note_vel, channel=pad_midi_channel)
        midi_usb.send(noteoff)





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
    # set all keys to trigger the blink callback
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
