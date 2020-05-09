# slstatus version
VERSION = 0

# customize below to fit your system

# paths
PREFIX = /usr
MANPREFIX = $(PREFIX)/share/man

X11INC = /usr/include/X11
X11LIB = /usr/lib/X11

# flags
CPPFLAGS = -I$(X11INC) -D_DEFAULT_SOURCE
CFLAGS   = -g -std=c99 -pedantic -Wall -Wextra
LDFLAGS  = -L$(X11LIB)
LDLIBS   = -lX11

# compiler and linker
CC = cc
