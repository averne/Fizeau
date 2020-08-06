#!/bin/python3

import os, sys
import math
from PIL import Image

MIN_TEMP = 1000
MAX_TEMP = 6500

clamp = lambda v, minv, maxv: max(minv, min(v, maxv))

def temp_to_col(temp):
    clamp(temp, MIN_TEMP, MAX_TEMP)

    temp /= 100.0

    if temp <= 66:
        red = 255
    else:
        red = clamp(329.698727446 * math.pow(temp - 60, -0.1332047592), 0, 255)

    if temp <= 66:
        green = clamp(99.4708025861 * math.log(temp) - 161.1195681661, 0, 255)
    else:
        tmp_green = 288.1221695283 * math.pow(temp - 60, -0.0755148492)
        green = clamp(tmp_green, 0, 255)

    if temp >= 66:
        blue = 255
    elif temp <= 19:
        blue = 0
    else:
        blue = clamp(138.5177312231 * math.log(temp - 10) - 305.0447927307, 0, 255)

    return int(red), int(green), int(blue)

def main(argc, argv):
    im = Image.new("RGB", (1000, 1000))
    px = im.load()
    for y, temp in enumerate(range(MIN_TEMP, MAX_TEMP, 100)):
        col = temp_to_col(temp)
        print(f"{temp} => r: {col[0] / 256}, g: {col[1] / 256}, b: {col[2] / 256}")
        for x in range(im.size[0]):
            px[x, y] = col
    im.show()

if __name__ == "__main__":
    main(len(sys.argv), sys.argv)
