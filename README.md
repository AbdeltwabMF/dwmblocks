# dwmblocks (Text Feed Generator)

Modular status bar for `dwm` written in c.

## Features & Patches

- [signal handling](https://dwm.suckless.org/patches/statuscmd/): Allowing to update blocks when receiving a signal.
- [clickable](https://dwm.suckless.org/patches/statuscmd/): Clicking on a block will set `BUTTON` variable with the number of clicked button.


## Modifying blocks

The statusbar is made from text output from command line programs.
Blocks are added and removed by editing the `config.h` file.
