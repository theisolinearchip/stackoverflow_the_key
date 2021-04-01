# Functional implementation for "The Key" (Stack Overflow copy-paste keyboard for April Fool 2021)

The Stack Overflow April Fool 2021 post was about [The Key](https://stackoverflow.blog/2021/03/31/the-key-copy-paste/), a three-keys keyboard that only allows copy and paste operations (classic Stack Overflow joke).

This code works with an __attiny85__ (or probably whatever similar AVR microcontroller once the proper ports, pins, fuses and speeds are changed) and turns the micro into a keyboard with only three buttons:

* Type "stackoverflow.com" + ENTER
* Copy
* Paste

It uses the **v-usb** library (with the proper hardware hooked up to be detected as a low-speed usb device) and three push buttons for the user.

The main file is a basic keyboard example (enable interrupts, create report, send report, repeat) from the **v-usb** documentation (not an expert on this library, but as far as I know it works pretty well!) with the three buttons logic.

## Some extra considerations

Since I'm using an __attiny85__ I needed to change a few things in order to make the __v-usb library__ work with it.
[This post helps me a lot](https://www.ruinelli.ch/how-to-use-v-usb-on-an-attiny85).

Some changes need to be done to modify the CPU speed (also fuses!) among with the use of an Oscillator Calibration function.

The input pins are also different than the ones in the post, since I was using code from another project ([NES Mini Controller USB Adapter for attiny85](https://github.com/theisolinearchip/nesmini_usb_adapter)) I wrote some months ago. In order to avoid conflict between the "USB pins" and the "I2C pins" (not used here) the _usbconfig.h_ re-arranges the DATA+ and DATA- pins from the regular ones to a different pair. Check that file for more info.

## Useful links

[https://hackaday.io/project/178652-stack-overflow-the-key-keyboard](https://hackaday.io/project/178652-stack-overflow-the-key-keyboard) This project on hackaday.io

[https://stackoverflow.blog/2021/03/31/the-key-copy-paste/](https://stackoverflow.blog/2021/03/31/the-key-copy-paste/) The Key on Stack Overflow blog (2021/03/31)

[https://www.obdev.at/products/vusb/index.html](https://www.obdev.at/products/vusb/index.html) The V-USB library, software implementation for low-speed
USB device on AVR microcontrollers

[https://www.ruinelli.ch/how-to-use-v-usb-on-an-attiny85](https://www.ruinelli.ch/how-to-use-v-usb-on-an-attiny85) How to make the original V-USB library
examples work in an attiny85

[https://thewanderingengineer.com/2014/08/11/pin-change-interrupts-on-attiny85/](https://thewanderingengineer.com/2014/08/11/pin-change-interrupts-on-attiny85/)
Pin change Interrupts on attiny85
