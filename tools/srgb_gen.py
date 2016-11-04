#
#  srgb_gen.py
#  tools/
#
#  Created by Ryan Huffman on 8/8/2016.
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

# Generates a lookup table for SRGB to Linear color transformations

NUM_VALUES = 256
srgb_to_linear = []

# Calculate srgb to linear
for i in range(NUM_VALUES):
    s = float(i) / 255
    if s < 0.04045:
        l = s / 12.92
    else:
        l = ((s + 0.055) / 1.055) ** 2.4
    srgb_to_linear.append(l)

# Format and print
data = "{\n    "
for i, v in enumerate(srgb_to_linear):
    data += str(v) + "f"
    if i < NUM_VALUES - 1:
        data += ", "
    if i > 0 and i % 6 == 0:
        data += "\n    "
data += "\n}"
print(data)
