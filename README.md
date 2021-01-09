# kindle-doom -- a port of PrBoom for the Kindle 4

## Description

This repository contains the source code for a port of the [PrBoom](http://prboom.sourceforge.net/) project to the Kindle 4. It is **not** intended as anything more than a proof-of-concept: the Kindle's e-ink display is absolutely not adapted to gaming.

## Installing & playing

Prerequisites: copy your `doom.wad` and `prboom.wad` to the root of your Kindle USB partition. `prboom.wad` can be found [here](data/prboom.wad).

1. Download the latest release
2. Make sure you installed [KUAL](https://www.mobileread.com/forums/showthread.php?t=203326) on your Kindle.
3. Extract the release to the root of your Kindle USB partition
4. Run KUAL and run "Simon's Fun Land -> PrBoom"

The controls are as follows:
- Keypad to move
- "OK" button to shoot
- "Menu" to open doors / activate switches
- "Back" to toggle the menu (mapped to Escape)
- "Keyboard" to select an entry in the menu (mapped to Enter)
- "Home" button to quit

## Compilation

If you want to build the project for yourself, you will need an `armv7` GCC toolchain. I successfully used Bootlin's [`armv7-eabihf`](https://toolchains.bootlin.com/releases_armv7-eabihf.html) "musl" toolchain. Compiling with the glibc toolchain also works, but the `libc` on the Kindle is too old and it will refuse to start.

Export `CC` to the location of the GCC cross-compiler and run `make`.

```shell
$ export CC=/home/simon/armv7-eabihf--musl--stable-2020.08-1/bin/arm-linux-gcc
$ make -j8
```

## Changes

Based on [`prboom-2.5.0`](https://sourceforge.net/projects/prboom/files/prboom%20stable/2.5.0/).

The main change is the addition of `src/KINDLE/`, which defines most Kindle-specific functions. I basically copied `src/SDL/` and modified everything to remove dependencies on SDL and instead work directly on the Kindle's framebuffer.

Resolution is hardcoded to 800x600 widescreen and cannot be changed.

I removed all the autoconf stuff and wrote a Makefile by hand.

## Known bugs

- Demo playback doesn't seem to work correctly. I think this is due to tick calculation, but I'm not sure.
- Because of this, it is not really possible to load the game directly, you have to "warp" to the beginning of the first level.

## Limitations

- The code is horrible, I only tried to hack together something that works.
- There's no sound support, because my Kindle doesn't have speakers / aux output.
- The game is basically unplayable because of the e-ink refresh mode, but it's still a nifty demo nevertheless.