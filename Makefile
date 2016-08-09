# Makefile for FreeBSD using LLVM/clang
#
# Created by Dr. Rolf Jansen on 2016-07-15.
# Copyright (c) 2016. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
# Usage examples:
#   make
#   make clean
#   make update
#   make install clean
#   make clean install CDEFS="-DDEBUG"

.if exists(.svn) || exists(../.svn) || exists(../../.svn) || exists(../../../.svn)
.ifmake update
REVNUM != svn update > /dev/null; svnversion
.else
REVNUM != svnversion
.endif
.else
REVNUM != cut -d= -f2 svnrev.xcconfig
.endif

CC       ?= clang
CFLAGS    = $(CDEFS) -DSVNREV=\"$(REVNUM)\" -std=c11 -g0 -Ofast -mssse3 -Wno-parentheses -Wno-empty-body
LDFLAGS   = -lm
PREFIX   ?= /usr/local

HEADER    = store.h
SOURCES   = store.c ipdb.c geoip.c
OBJECTS   = $(SOURCES:.c=.o)

all: $(HEADER) $(SOURCES) $(OBJECTS) ipdb geoip

depend:
	$(CC) $(CFLAGS) -E -MM *.c > .depend

$(OBJECTS):
	$(CC) $(CFLAGS) $< -c -o $@

ipdb: $(OBJECTS)
	$(CC) store.o ipdb.o $(LDFLAGS) -o $@

geoip: $(OBJECTS)
	$(CC) store.o geoip.o $(LDFLAGS) -o $@

clean:
	rm -rf *.o *.core ipdb geoip

update: clean all

install: ipdb geoip
	install -m 555 -s ipdb ${PREFIX}/bin/ipdb
	install -m 555 -s geoip ${PREFIX}/bin/geoip
	install -m 555 ipdb-update.sh ${PREFIX}/bin/ipdb-update.sh
	install -m 555 ipdbtools.1 ${PREFIX}/man/man1/ipdbtools.1