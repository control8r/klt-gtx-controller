# KLT ArtNet Controller

Minimal adapter controlling Schauf KLT displays via ArtNet.

## Configuration

To be done.

For an example configuration, see [example.conf](example.conf)

## Building

Run `make` in the project directory on a system with a working C compiler.

## Usage

The `controller` binary requires the configuration file as first and only argument.

The controller uses data from three ArtNet channels and maps them as follows:

* Channel 1: Message index
* Channel 2: Message timeout / tempo (out of 9 different time settings)
* Channel 3: Transition function (out of 16 variants)
