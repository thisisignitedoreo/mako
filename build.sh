#!/bin/sh

set -e

if test ! -f strap/libstrap.a; then
    strap/build.sh && cd .
fi

CC=gcc
CFLAGS="-Wall -Wextra -Werror -std=gnu99 -pedantic -L./strap/ -I./strap/src/"
LIBS="-lstrap"

$CC $CFLAGS -o mako src/main.c $LIBS
