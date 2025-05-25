# Control application for AceMagic S1 LCD display

This is a very small and limited application I use to make the LCD display
on my AceMagic S1 mini PC fully black as it lives in my bedroom and serves
as a file server. The application is written in pure C and depends on libusb
only.

It may well get more features if I have more time.

In case you want to experiment with making the app more useful the protocol
used by the LCD display is described
[here](https://github.com/tjaworski/AceMagic-S1-LED-TFT-Linux) very well.

## How to build

For FreeBSD simply run
```
$ make
```

For Linux you'd need to have libusb installed and to tweak `Makefile` to
point your compiler to the correct place of libusb headers and shared objects.
