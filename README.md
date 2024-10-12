# Fizeau

<p align="center"><img src="https://github.com/averne/Fizeau/assets/45773016/1944246d-df65-461f-be2a-df3359476673" height=300></p>

Adjust the color of the screen of your Nintendo Switch.

# Features
  - Modify the color temperature, saturation and hue of the display.
  - Filter colors to one single component.
  - Apply color corrections: gamma, luminance, and color range.
  - Schedule settings to be applied to dusk/dawn time, with smooth transitions.
  - Configurable screen dimming.

# Images
<p float="left">
  <img src="https://github.com/averne/Fizeau/assets/45773016/028b1554-da6b-4efa-be76-af05ede066b0" width="400" />
  <img src="https://github.com/averne/Fizeau/assets/45773016/561e17a3-cd71-4d9e-bbee-1d2a9408df37" width="400" />
  <img src="https://github.com/averne/Fizeau/assets/45773016/b3ad91b6-3e6c-4d47-a2af-26ef58496aa7" width="400" />
  <img src="https://github.com/averne/Fizeau/assets/45773016/a5c4e32d-c7cd-4ef4-8298-d33e4984b79a" width="400" />
</p>

# Installation
Download the latest zip from the [release page](https://github.com/averne/Fizeau/releases/latest), unzip it to the root of your sd card (be careful to merge and not overwrite folders), and reboot.

Only the latest version of the [Atmosphère](https://github.com/Atmosphere-NX/Atmosphere) CFW is supported.

If you want to use the overlay, you will need to set up Tesla (install [Tesla-Menu](https://github.com/WerWolv/Tesla-Menu) and [ovlloader](https://github.com/WerWolv/nx-ovlloader)). This isn't supported on firmware versions prior to 9.0.0.

## Supported firmwares
Firmware versions 9.0.0 and later are supported. If you encounter an issue running this software on a particular version, please create an issue.

# Usage
You can refer to the built-in help. Navigate with either the touchscreen or the D-pad buttons.

## Settings
Settings are saved at `/switch/Fizeau/config.ini` and `/config/Fizeau/config.ini` (in order of priority), which you can also edit.

# Building
  - Compiling requires a working [devkitA64](https://devkitpro.org/wiki/devkitPro_pacman) installation, with package `switch-glm` installed. You also need `rsync` to be available (it should be by default on all major operating systems).
  - Clone this repository recursively (`git clone --recursive https://github.com/averne/Fizeau`)
  - Navigate to its directory (`cd Fizeau`).
  - Run `make dist`.
  - You will find the output file in `out/`.

# How it works
This software uses the CMU (Color Management Unit) built into the Tegra GPU of the Nintendo Switch. The purpose of this unit is to enable gamma correction/color gamut changes.

The CMU works in 3 passes:
- the first pass converts 8-bit sRGB data into a 12-bit linear colorspace, using a LUT (look-up table). Therefore, it also increases the precision of the color data.
- the second pass is a dot product between the CSC (Color Space Correction) matrix and the RGB data, as illustrated below.
  <p align="center"><img src="https://i.imgur.com/qrO2Xdo.png"></p>
- the third pass maps the 12-bit linear, now corrected color data back into 8-bit sRGB, using another LUT. Hence the precision after this pass is decreased. Moreover, this LUT is split into two parts: the first 512 entries map [0, 32), while the 448 other represent [32, 256). This allows greater precision for darker color components, which the human eye is more sensitive to.

Overview of the CMU pipeline:
  <p align="center"><img src="https://i.imgur.com/pbca2eH.png"></p>

More detail can be found in the TRM (Tegra Reference Manual), section 24.3.14 (Display Color Management Unit).

Official software use the CMU for multiple purposes (the following images were generated using a script found [here](misc/cmu.py), with data dumped from running official software):
- Represented below is the default CMU configuration. No correction is applied:
  <p align="center"><img src="https://i.imgur.com/bSfIao7.png"></p>
- Color inversion is applied using the LUT2, which is simply inverted:
  <p align="center"><img src="https://i.imgur.com/U7oSJYl.png"></p>
- Grayscale is implemented with the CSC, using the luminance function Y = 0.2126 * R + 0.7152 * G + 0.0722 * B on each component (the actual coefficients being 0.2109375, 0.7109375 and 0.0703125 due to the limited precision of CSC components which are 10-bit signed [Q1.8](https://en.wikipedia.org/wiki/Q_(number_format)) numbers):
  <p align="center"><img src="https://i.imgur.com/UfviH8k.png"></p>
- Lastly, luminance correction, here with a luma=1.0 (color range correction is very similar):
  <p align="center"><img src="https://i.imgur.com/fsLv1wr.png"></p>

In addition, the color range of the external display is restricted using the HDMI AVI (Auxiliary Video Information) infoframe.

Official software uses precalculated default gamma ramps, and apply modifications to those (the relevant function can be found at .text + 0x05c70 in 6.0.0 nvnflinger). However here gamma ramps are generated at runtime, which is both more elegant and easily enables non-default gamma. The generated LUT1 is byte-for-byte identical to the official one.

# Credits
  - [zakiph27](https://github.com/zakiph27) for the icon and testing.
