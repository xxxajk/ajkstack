#
# These are set for a mega 1280 + quadram plus my serial patch.
# If you lack quadram, or want to disable LFN, just change _FS_TINY=1 _USE_LFN=0
#
# If your board is a mega 2560 uncomment the following two lines
#BOARD = mega2560
#PROGRAMMER = wiring
# ...and then comment out the following two lines
#BOARD = mega
#PROGRAMMER = arduino

BOARD = teensypp2

# set your Arduino tty port here
PORT = /dev/ttyACM0

EXTRA_FLAGS = -D _USE_LFN=3

# change to 0 if you have quadram to take advantage of caching FAT
EXTRA_FLAGS += -D _FS_TINY=1


EXTRA_FLAGS += -D _MAX_SS=512


# Don't worry if you don't have external RAM, xmem2 detects this situation.
# You *WILL* be wanting to get some kind of external ram on your mega in order to
# do anything that is intense.
ifneq "$(BOARD)" "teensy31"
ifneq "$(BOARD)" "teensy3"
EXTRA_FLAGS += -D EXT_RAM_STACK=1
EXTRA_FLAGS += -D EXT_RAM_HEAP=1
EXTRA_FLAGS += -D USE_MULTIPLE_APP_API=16
endif
endif

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
