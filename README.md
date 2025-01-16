# Classic macOS applications:

## SlotsDump
For each slot, parse the Slot Manager data and dump the slot ROM to a binary file.

## SlotsGrab
For each slot, dump the slot ROM to a binary file.

# Modern macOS command line tools:

## SlotsParse
Parse the Slot Manager data that exists in a binary file. The address of the last byte of the ROM is translated to be between 0xFEFFFFFC and 0xFEFFFFFF, based on the valid byte lanes specified by the header. A future change could allow a parameter to specify different addresses or valid byte lanes.

# Notes
The ROM for slot 0 is actually part of the ROM of the Mac and is the same as the `DeclData` file created by the `tbxi dump` command.
