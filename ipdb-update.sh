#!/bin/sh

# Shell Script for updating the Geo-location databases by downloading
# the latest delegation statistics files of the 5 RIR's.
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


# verify the path to the fetch utility
if [ -e "/usr/bin/fetch" ]; then
   FETCH="/usr/bin/fetch"
elif [ -e "/usr/local/bin/fetch" ]; then
   FETCH="/usr/local/bin/fetch"
elif [ -e "/opt/local/bin/fetch" ]; then
   FETCH="/opt/local/bin/fetch"
else
   echo "No fetch utility can be found on the system -- Stopping."
   echo "On Mac OS X, execute either 'sudo port install fetch' or install"
   echo "fetch from source into '/usr/local/bin', and then try again."
   exit 1
fi

if [ "$1" == "" ]; then
#  ?IRROR="ftp.afrinic.net" -- as of 2016-07-15, AFRINIC does not mirror the data of the other RIR's
#  MIRROR="ftp.apnic.net"   -- the RIPE directory is named ripe-ncc instead of ripencc
#  ?IRROR="ftp.arin.net"    -- as of 2016-07-15, the data of the other RIR's seem to be heavily outdated
#  MIRROR="ftp.lacnic.net"
   MIRROR="ftp.ripe.net"
else
   MIRROR="$1"
fi

if [ $MIRROR == "" ]; then
   exit 1
fi

if [ $MIRROR == "ftp.afrinic.net" ] || [ $MIRROR == "ftp.arin.net" ]; then
   echo "ARIN and AFRINIC do not reliably mirror the data of the other RIR's -- Stopping."
   exit 1
fi

if [ $MIRROR == "ftp.apnic.net" ]; then
   RIPEDIR="ripe-ncc"
else
   RIPEDIR="ripencc"
fi

# Storage location of configuration files and the IP-Ranges database files
IPRanges="/usr/local/etc/ipdb/IPRanges"
if [ ! -d "$IPRanges" ]; then
   mkdir -p "$IPRanges"
fi

# AFRINIC IPv4 ranges
rm -f "$IPRanges/afrinic.md5"
$FETCH -o "$IPRanges/afrinic.md5" "ftp://$MIRROR/pub/stats/afrinic/delegated-afrinic-extended-latest.md5" && \
$FETCH -o "$IPRanges/afrinic.dat" "ftp://$MIRROR/pub/stats/afrinic/delegated-afrinic-extended-latest"
if [ -f "$IPRanges/afrinic.md5" ] && [ -f "$IPRanges/afrinic.dat" ]; then
   stored_md5=`/usr/bin/cut -d " " -f4 "$IPRanges/afrinic.md5"`
   actual_md5=`/sbin/md5 -q "$IPRanges/afrinic.dat"`
   if [ "$stored_md5" != "$actual_md5" ]; then
      exit 1
   fi
else
   exit 1
fi

# APNIC IPv4 ranges
rm -f "$IPRanges/apnic.md5"
$FETCH -o "$IPRanges/apnic.md5" "ftp://$MIRROR/pub/stats/apnic/delegated-apnic-extended-latest.md5" && \
$FETCH -o "$IPRanges/apnic.dat" "ftp://$MIRROR/pub/stats/apnic/delegated-apnic-extended-latest"
if [ -f "$IPRanges/apnic.md5" ] && [ -f "$IPRanges/apnic.dat" ]; then
   stored_md5=`/usr/bin/cut -d " " -f4 "$IPRanges/apnic.md5"`
   actual_md5=`/sbin/md5 -q "$IPRanges/apnic.dat"`
   if [ "$stored_md5" != "$actual_md5" ]; then
      exit 1
   fi
else
   exit 1
fi

# ARIN IPv4 ranges
rm -f "$IPRanges/arin.md5"
$FETCH -o "$IPRanges/arin.md5" "ftp://$MIRROR/pub/stats/arin/delegated-arin-extended-latest.md5" && \
$FETCH -o "$IPRanges/arin.dat" "ftp://$MIRROR/pub/stats/arin/delegated-arin-extended-latest"
if [ -f "$IPRanges/arin.md5" ] && [ -f "$IPRanges/arin.dat" ]; then
   stored_md5=`/usr/bin/cut -d " " -f1 "$IPRanges/arin.md5"`
   actual_md5=`/sbin/md5 -q "$IPRanges/arin.dat"`
   if [ "$stored_md5" != "$actual_md5" ]; then
      exit 1
   fi
else
   exit 1
fi

# LACNIC IPv4 ranges
rm -f "$IPRanges/lacnic.md5"
$FETCH -o "$IPRanges/lacnic.md5" "ftp://$MIRROR/pub/stats/lacnic/delegated-lacnic-extended-latest.md5" && \
$FETCH -o "$IPRanges/lacnic.dat" "ftp://$MIRROR/pub/stats/lacnic/delegated-lacnic-extended-latest"
if [ -f "$IPRanges/lacnic.md5" ] && [ -f "$IPRanges/lacnic.dat" ]; then
   stored_md5=`/usr/bin/cut -d " " -f4 "$IPRanges/lacnic.md5"`
   actual_md5=`/sbin/md5 -q "$IPRanges/lacnic.dat"`
   if [ "$stored_md5" != "$actual_md5" ]; then
      exit 1
   fi
else
   exit 1
fi

# RIPENCC IPv4 ranges
rm -f "$IPRanges/ripencc.md5"
$FETCH -o "$IPRanges/ripencc.md5" "ftp://$MIRROR/pub/stats/$RIPEDIR/delegated-ripencc-extended-latest.md5" && \
$FETCH -o "$IPRanges/ripencc.dat" "ftp://$MIRROR/pub/stats/$RIPEDIR/delegated-ripencc-extended-latest"
if [ -f "$IPRanges/ripencc.md5" ] && [ -f "$IPRanges/ripencc.dat" ]; then
   stored_md5=`/usr/bin/cut -d " " -f4 "$IPRanges/ripencc.md5"`
   actual_md5=`/sbin/md5 -q "$IPRanges/ripencc.dat"`
   if [ "$stored_md5" != "$actual_md5" ]; then
      exit 1
   fi
else
   exit 1
fi

/usr/local/bin/ipdb "$IPRanges/ipcc.bst" \
                    "$IPRanges/afrinic.dat" \
                    "$IPRanges/apnic.dat" \
                    "$IPRanges/arin.dat" \
                    "$IPRanges/lacnic.dat" \
                    "$IPRanges/ripencc.dat"
