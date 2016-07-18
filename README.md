# ipdb
IPv4 Geo-Location database tools and ipfw divert filter daemon for FreeBSD

This project provide all the tools for IPv4-Geo-Blocking at the firewall level with ipfw on FreeBSD.

Quick Start

1. Download and the ipdb project directory from GitHub.


2. cd into the directory
   $ cd ~/install/ipdb


3. run make
   $ make


4. as user root, install the tools and the ipfw divert filter daemon
   $ sudo make install clean

or # make install clean

   the following tools and files will be created and installed into /usr/local/bin or /usr/local/etc/rc.d
   - ipdb-update.sh    # a shell script file for updateing the geoip database by
                       # downloading the latest delegation statistics files of the 5 RIR's.
  
   - ipdb              # a tool for consoliting the IPv4 ranges from statistics file into
                       # a sorted binary file suitable for direct reading it into a
                       # completely balanced binary search tree be the lookup tool and
                       # and the ipfw divert filter daemon.
                      
   - geoip             # a tool for manually looking up an IPv4 address on the command line.

   - geod              # the ipfw divert filter daemon.
   - geod.rc           # the rc script of geod, will be copied to /usr/local/etc/rc.d/geod


5. First download the delegation statistic files of the 5 Regional Internet Registries (RIR's), i.e.:
   AFRINIC, APNIC, ARIN, LACNIC, RIPENCC. In theory all the RIR's should mirror the files of each other,
   in practice only the mirrors of the Asia Pacific, the Latin America and the European RIR's are useful.
   
   Choose one of the three useful mirror sites, depending on where you are located:
   - ftp.apnic.net    # Asia Pacific
   - ftp.lacnic.net   # Latin America
   - ftp.ripencc.net  # Europe and Eurasia
   
   Start the ipdb-update.sh shell script as user root, passing (n)one of the above mirror ftp domains
   as the commandline parameter. RIPENCC is the default mirror, and that doamin may be omitted.
   
     # ipdb-update.sh ftp.apnic.net
   
  This will download teh statistic files together with the MD5 verification hashes into:
  /usr/local/etc/ipdb/IPRanges/. Said directory will becreated if it does not exit. If the
  downloads went smooth, the script will start the ipdb tool in order to generate right in 
  the same go the binary file with the consolidated IPv4 ranges.


6. Check whether the database is ready by looking up some IPv4 addresses using the geoip tool.

   $ geoip 62.175.157.33
   62.175.157.33 in 62.174.0.0-62.175.255.255 in ES
   
   $ geoip 141.33.17.2
   141.33.17.2 in 141.12.0.0-141.80.255.255 in DE
   
   $ geoip 99.67.80.80
   99.67.80.80 in 98.160.0.0-99.191.255.255 in US
   
   $ geoip 192.168.1.1
   192.168.1.1 not found
   

7. If not already done activate ipfw


8. In addtion to the ipfw modules, the ipdivert kernel module needs to be loaded
   WARNING: Do this only after ipfw is setup and running, otherwise it may happen
            that you inadvertently lock out yourself by the following.

   # echo 'ipdivert_load="YES"' >> /boot/loader.conf
   # kldload ipdivert.ko


9. Add the lines for starting the geo-blocking ipfw divert filter daemon to /etc/rc.conf

   # echo 'geod_load="YES"'
   
   Configuration examples (use either the -a or the -d flag into one _flags line in /etc/rc.conf):
   
   - Allow all diverted packets from Germany, Brazil and the US, and deny everything else:
   # echo 'geod_flags="-a DE:BR:US"'
   
   - Deny all diverted packets from North Korea, Turkey and Great Britain, and allow everything else:
   # echo 'geod_flags="-d KO:TR:GB"'
   
   - You may add any number of capital letter ISO country codes separated by colons.

  # service geod start


10. Add the geod divert rule to your ipfw rule set:

Examples:

TCP only filter
   ipfw add 70 divert 8669 tcp from any to any 80,443 in recv WAN_if setup

TCP and UDP filter
   ipfw add 70 divert 8669 ip4 from any to any 53,80,443,500,587,4500 in recv WAN_if


Done.

