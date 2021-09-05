# VocalFusionDriver
Raspberry Pi VocalFusion linux driver for kernel 5.10 (possibly 5.x)

## How to test / use

Compile the module in the driver sub-folder and install it. (better Makefile and instructions will follow shortly)
- make all
- copy the driver to /lib/modules/....
- depmod -a

Copy over the dtbo overlay(s) to the /boot/overlay folder (depends a bit on the used distro)
- cp xvf3510.dtbo /boot/overlays/

Add the following section to the config.txt
```dtoverlay=xvf3510```

## What is already done / implemented

- xvf3510 dtoverlay, will initiate the driver loading
- sets up the mclk at gpio 4 at configurable rate from the dtoverlay
- configures the reset and power gpio to turn it on

## What still to do (when time allows it)

- Make the rpi bitclock master/slave flag (SND_SOC_DAIFMT_CBS_FLAG) configurable in dtoverlays
- Make the reset and power gpio numbers configurable in dtoverlays
- Poll the right hardware (rpi0, 2, 3 and 4 support) to auto select the right i2s address (CARD_PLATFORM_STR)
- Migrate the reset python script over to the driver.

With above changes I can provide dtoverlays for all the different I2S based hats (3610/3510/3500/3000). You then just have to load this one driver module by loading the right dtoverlay within config.txt to initiate and load the soundcard.
