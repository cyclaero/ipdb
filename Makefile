# Makefile for FreeBSD using LLVM/clang
#
# Created by Dr. Rolf Jansen on 2016-07-15.
# Copyright © 2016-2018 Dr. Rolf Jansen. All rights reserved.
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

CC     ?= clang
CFLAGS ?= -g0 -O3

.if exists(.git)
.ifmake update
REVNUM != git pull origin >/dev/null 2>&1; git rev-list --count HEAD
.else
REVNUM != git rev-list --count HEAD
.endif
MODCNT != git status -s | wc -l
.if $(MODCNT) > 0
MODIED  = M
.else
MODIED  =
.endif
.else
REVNUM != cut -d= -f2 scmrev.xcconfig
.endif

.if $(MACHINE) == "i386" || $(MACHINE) == "amd64" || $(MACHINE) == "x86_64"
CFLAGS += $(CDEFS) -mssse3
.elif $(MACHINE) == "arm"
CFLAGS += $(CDEFS) -fsigned-char
.else
CFLAGS += $(CDEFS)
.endif

CFLAGS += -DSCMREV=\"$(REVNUM)$(MODIED)\" -std=gnu11 -fstrict-aliasing -fno-common -Wno-multichar -Wno-parentheses -Wno-empty-body
LDFLAGS = -lm
PREFIX ?= /usr/local

HEADERS = utils.h uint128t.h store.h
SOURCES = utils.c uint128t.c store.c ipup.c ipdb.c
OBJECTS = $(SOURCES:.c=.o)

all: $(HEADERS) $(SOURCES) $(OBJECTS) ipup ipdb

depend:
	$(CC) $(CFLAGS) -E -MM *.c > .depend

$(OBJECTS): Makefile
	$(CC) $(CFLAGS) $< -c -o $@

ipup: $(OBJECTS)
	$(CC) utils.o uint128t.o store.o ipup.o $(LDFLAGS) -o $@

ipdb: $(OBJECTS)
	$(CC) utils.o uint128t.o store.o ipdb.o $(LDFLAGS) -o $@

clean:
	rm -rf *.o *.core ipup ipdb

update: clean all

install: ipdb ipup
	install -m 555 -s ipup $(DESTDIR)${PREFIX}/bin/ipup
	install -m 555 -s ipdb $(DESTDIR)${PREFIX}/bin/ipdb
	install -m 555 ipdb-update.sh $(DESTDIR)${PREFIX}/bin/ipdb-update.sh
	install -m 555 ipdbtools.1 $(DESTDIR)${PREFIX}/man/man1/ipdbtools.1
	ln -f -s ipdbtools.1 $(DESTDIR)${PREFIX}/man/man1/ipup.1
	ln -f -s ipdbtools.1 $(DESTDIR)${PREFIX}/man/man1/ipdb.1
	ln -f -s ipdbtools.1 $(DESTDIR)${PREFIX}/man/man1/ipdb-update.sh.1
