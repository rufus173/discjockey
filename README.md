
# What is it

It is a terminal based music player, that can load individual or directories of songs and optionaly shuffle them. It has an interface for selecting songs and optional autoplay, with a status bar to display what is playing, and a nonsensical visualiser based off the raw audio data.
But, the best way to see what it is is to use it.

# Requirements

`libsdl2-mixer-dev`
`libncurses-dev`

# Install

`make discjockey`
`sudo ./install`

# Usage

Specify files or directories. Searches recursively, and sorts alphabetically within each directory.
Designed so that if you number your files upfront, such as `1-a.wav` or `23 b.mp3` or `5. c.ogg` or `07 d.mp3` files within the same directory will be sorted in numerical order when loaded.
Use `discjockey --help` to see controls and command line arguments
