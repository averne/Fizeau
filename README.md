# Fizeau

<p align="center"><img src="https://github.com/averne/Fizeau/blob/master/application/icon.jpg?raw=true"></p>

Adjust the color of the screen of your Nintendo Switch.

# Overview

## Temperature/color:
  - Use the temperature slider to select the desired shading.
  - You can override the color by using the RGBA sliders.
  - The "Alpha" value controls the opacity of the layer.
  - Due to the way Fizeau works, it is sadly not possible to directly modify
        the screen color. Instead, it creates a transparent layer and applies
        color to it. However, this can lead to an overall increase of luminosity,
        especially visible with dark colors.
  - Additionally, the layer is not visible in screenshots/recordings.

## Dawn/dusk:
  - When set, Fizeau will start at the "dusk" hour and shutdown at the "dawn"
        hour.
  - You can override this by using the checkbox. Fizeau will then stay in the
        specified state.

## Settings:
  - Settings are saved at /switch/Fizeau/config.ini, which you can also edit.
  - To reduce the memory usage of the sysmodule, settings are not read
        continually. Instead, they are applied on application launch.
  - Thus, you will need to launch the client after a reboot to restart Fizeau.

# Images

<p align="center"><img src="https://i.imgur.com/bJ3xi86.gif"></p>
<p align="center"><img src="https://i.imgur.com/6t26Ufa.jpg"></p>
<p align="center"><img src="https://i.imgur.com/I4yov0m.jpg"></p>

# Installing
  - Download the [latest release](https://github.com/averne/Fizeau/releases), unzip it to the root of your SD card, and reboot.
  - Only the latest version of the [Atmosphère](https://github.com/Atmosphere-NX/Atmosphere) CFW is supported.

# Building
  - Compiling requires a working [devkitA64](https://devkitpro.org/wiki/devkitPro_pacman) installation, with packages `switch-mesa` & `switch-libdrm_nouveau`.
  - Clone this repository recursively (`git clone --recursive https://github.com/averne/Fizeau`)
  - Navigate to its directory (`cd Fizeau`).
  - Run `make dist`.
  - You will find output file in `out/`.

# Credits
  - [zakiph27](https://github.com/zakiph27) for the icon.
  - [TheGreatRambler](https://github.com/TheGreatRambler) for the gif recording.
  - [xx](https://github.com/neonsea) because it's him.
  - [Paul_GD](github.com/PaulGameDev) for mental assistance.
