NUM_VALUES = 256
srgb_to_linear = []

# Calculate srgb to linear
for i in range(NUM_VALUES):
    s = float(i) / 255
    if s < 0.04045:
        l = s / 12.92
    else:
        l = ((s + 0.044) / 1.055) ** 2.4
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
