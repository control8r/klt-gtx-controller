# KLT ArtNet Controller

Minimal adapter controlling Schauf KLT displays via ArtNet.

## Configuration

The configuration file is a simple text file arranged in `[sections]`.
Each section configures different parts of the controller.

For an example configuration, see [example.conf](example.conf)

### The `display` section

This section configures how the controller talks to the display.

The following options are required in this section:

* `lines <number of lines>`: The number of text lines the display supports.
	Passed through to the display directly, not used within the controller.
* `destination <type> <target>`: Connection to the display. *type* can be either
	`network` (*target* then being `<ip> <port>`) or `device` (*target* being
	a device node such as `/dev/ttyUSB0`).

The following options can optionally be specified in this section:

* `address <id>`: Line-protocol display address. `0` is broadcast to all connected
	displays (default).
* `width <characters>`: The line width (in characters) of the display.
	This parameter is used to automatically pad lines in configured messages.
	When controlling single-line displays, this parameter is irrelevant.

### The `artnet` section

This section configures how the controller receives ArtNet control data.

The following options are required in this section:

* `listen <host> <port>`: Address and port to listen on for ArtNet data.

The following options can optionally be specified in this section:

* `net <net>`: ArtNet universe high byte (network).
* `universe <uni>`: ArtNet universe low byte (universe index).
* `address <addr>`: ArtNet address of the first channel (zero-indexed).

### `message` sections

Each message to be displayed is configured using a `[message <from>-<to>]` section.
Messages are activated when the first ArtNet channel hits a value between *from*
and *to*. The parameter *to* (and the dash separating them) can optionally be omitted.

Messages are configured verbatim line-by-line. If the `display` parameter *width* is
set, lines are automatically padded to this width using space characters.
For single-line displays, this can be ignored. If *width* is not configured (or
configured as 0), newlines in the message will be ignored and padding must be
performed manually (using spaces in the configuration).

Messages are split into *pages* by inserting blank lines. The display will automatically
switch between pages in an interval configured using ArtNet channel 2 using an effect
configured by channel 3. For more information on these effects, see the display
documentation.

## Building

Run `make` in the project directory on a system with a working C compiler.

## Usage

The `controller` binary requires the configuration file as first and only argument.

The controller uses data from three ArtNet channels and maps them as follows:

* Channel 1: Message index
* Channel 2: Message timeout / tempo (out of 9 different time settings)
* Channel 3: Transition function (out of 16 variants)

Data is only transmitted to the display when required, ie. when first receiving ArtNet
input and subsequently when channel 0 hits a value that is mapped to a different message
than the previous value.

This implies that changes to the tempo and transition function channels are only applied
when a new message is transmitted to the display.
