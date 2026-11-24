# References {#references}

## Hardware and architecture
* [BCM2711 ARM Peripherals (Raspberry Pi Ltd, rev. 4)](https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf)
* [Arm Architecture Reference Manual (version M.d)](https://support.arm.com/documentation/ddi0487/md)
* [Arm Generic Interrupt Controller Architecture Specification v2.0 (IHI 0048B.b)](https://support.arm.com/documentation/ihi0048/bb/)
* [CoreLink GIC-400 Generic Interrupt Controller TRM (version r0p1)](https://developer.arm.com/documentation/ddi0471/latest)
* [Cortex-A72 MPCore Processor TRM (version r0p3)](https://support.arm.com/documentation/100095/0003/)
* [Raspberry Pi firmware: Mailbox property interface (wiki)](https://github.com/raspberrypi/firmware/wiki/)
* [DTB format](https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html)

## Concepts
* [Operating Systems: Three Easy Pieces, Arpaci-Dusseau](https://pages.cs.wisc.edu/~remzi/OSTEP/)
* [OSDev Wiki (the Raspberry Pi 4 boot code started from here)](https://wiki.osdev.org/Raspberry_Pi_Bare_Bones)
* [The little book about OS development](https://littleosbook.github.io/)

## Tutorials
* [Ohyaan's tutorial](https://ohyaan.github.io/os-development/)
* [sypstraw's tutorial (mailbox code adapted mainly from here)](https://rpi4os.com/)

## Code consulted
* Linux [`bcm2835_wdt.c`](https://github.com/torvalds/linux/blob/master/drivers/watchdog/bcm2835_wdt.c) (watchdog, reboot and shutdown via the PM registers)
* Linux [`bcm2835-mailbox.c`](https://github.com/torvalds/linux/blob/master/drivers/mailbox/bcm2835-mailbox.c) and [`raspberrypi-firmware.h`](https://github.com/torvalds/linux/blob/master/include/soc/bcm2835/raspberrypi-firmware.h) (mailbox registers and property tags)

## Tools and third-party files
* [Doxygen Awesome CSS by jothepro (MIT)](https://github.com/jothepro/doxygen-awesome-css)
