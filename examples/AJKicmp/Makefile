#
# These are set for a teensy++ 2.0 + quadram.
#BOARD = mega2560
#BOARD = mega
BOARD = teensypp2

# set your tty port here
PORT = /dev/ttyACM0

EXTRA_FLAGS = -D _USE_LFN=3

# change to 0 if you have quadram to take advantage of caching FAT
EXTRA_FLAGS += -D _FS_TINY=1


EXTRA_FLAGS += -D _MAX_SS=512


# Don't worry if you don't have external RAM, xmem2 detects this situation.
# You *WILL* be wanting to get some kind of external ram on your mega in order to
# do anything that is intense.
EXTRA_FLAGS += -D EXT_RAM_STACK=1
EXTRA_FLAGS += -D EXT_RAM_HEAP=1
EXTRA_FLAGS += -D USE_MULTIPLE_APP_API=16

EXTRA_FLAGS += -D USB_HOST_SERIAL=Serial1
EXTRA_FLAGS += -D USB_HOST_SERIAL_NUM=1 
EXTRA_FLAGS += -D SLIPDEV=Serial
EXTRA_FLAGS += -D CONSOLE=1
EXTRA_FLAGS += -D CONSOLE_BAUD_RATE=115200

#
# Advanced debug
#

#EXTRA_FLAGS += -D DEBUG_USB_HOST

# The following are the libraries used.
include ../Arduino_Makefile_master/_Makefile.master
