ICSC Library for Linux
======================

# WORK IN PROGRESS

This is an implementation of the ICSC serial communication protocol
for Linux. It is aimed at the Raspberry Pi and BeagleBone Black, but
other embedded Linux system will also work.

Building
--------

    $ autoreconf -fi
    $ ./configure
    $ make
    $ sudo make install

By default it installs into /usr/local/lib and /usr/local/include. To install it in
/usr instead, change the configure command:

    $ ./configure --prefix=/usr
    $ make
    $ sudo make install

It is possible to cross-compile the library on a Linux PC if you have the
Raspberry Pi toolchain installed:

    $ ./configure --host=arm-linux-gnueabihf --prefix=/path/to/place/to/put/it
    $ make
    $ make install

You can then SCP the files from /path/to/place/to/put/it to your Raspberry Pi.
