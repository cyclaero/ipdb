See also the man file at: [**Tools for IP based Geo-blocking and Geo-routing**](https://cyclaero.github.io/ipdb/)

## Latest & hotest case study

### Opting out of the EU's General Data Protection Regulation by Geo Blocking the EU

The [EU-GDPR - 88 pages of the lawyers finest, compressed by 9.6 pt EUAlbertina](https://eur-lex.europa.eu/legal-content/EN/TXT/PDF/?uri=CELEX:32016R0679) is going to become effective on May 25th, 2018. One option is to opt-out of this bullshit by Geo-Blocking th EU.

On the FreeBSD gateway of your internet service to be hidden from EU citizens, do the following:
1. `pkg install ipdbtools`,
2. `ipdb-update.sh`,
3. add the following to your IPFW directives - take care to place this before any other rules allowing any web traffic: 

    ...
    # EU-GDPR Geo blocking using an ipfw table
    /sbin/ipfw -q table 66 create
    /usr/local/bin/ipup -t AL:AT:BE:BG:CY:CZ:DE:DK:EE:ES:FI:FR:GB:GR:HR:HU:IE:IT:LT:LU:LV:ME:MK:MT:NL:PL:PT:RO:RS:SE:SI:SK:TR -n 66 -4 | /sbin/ipfw -q /dev/stdin
    /sbin/ipfw -q add 66 deny tcp from table\(66\) to any 80,443 in recv $WAN setup
    ...


## Geo-blocking at the Firewall

In general, access control by the firewall is established by selectors that can be attributed to incoming and outgoing IP-packets, like physical interfaces on which the packets are going, source and target IP addresses, protocol types, port numbers, content types and content, etc. The geo-location would be just another selector, but this information is not carried explicitly with the IP packets, however, it can be obtained using the source IP address as a key for looking-up the location in a geo-database. For example, besides other information, the country to which the IPv4 address I00.0.0.1 is delegated, can be obtained with the common unix tool `whois`:

    $ whois 100.0.0.1
    >>>>
    ...
    NetRange:       100.0.0.0 - 100.41.255.255
    CIDR:           100.32.0.0/13, 100.40.0.0/15, 100.0.0.0/11
    NetName:        V4-VZO
    NetHandle:      NET-100-0-0-0-1
    Parent:         NET100 (NET-100-0-0-0-0)
    NetType:        Direct Allocation
    OriginAS:       AS19262
    Organization:   MCI Communications Services, Inc. d/b/a Verizon Business (MCICS)
    RegDate:        2010-12-28
    Updated:        2016-05-17
    Ref:            https://whois.arin.net/rest/net/NET-100-0-0-0-1

    OrgName:        MCI Communications Services, Inc. d/b/a Verizon Business
    OrgId:          MCICS
    Address:        22001 Loudoun County Pkwy
    City:           Ashburn
    StateProv:      VA
    PostalCode:     20147
    Country:        US
    ...

`whois` does an online lookup in the databases of the [5 Regional Internet Registries](https://en.wikipedia.org/wiki/Regional_Internet_registry) (`AFRINIC, APNIC, ARIN, LACNIC, RIPENCC)`, and this is the most reliable way to obtain the country code for an IP address, because the RIR's are the authorities for internet number delegations.

Unfortunately, online database look-up is by far too slow for even thinking about being utilized on the firewall level, where IP packets need to be processed in a 10th or a 100th of a millisecond. Therefore a locally maintained Geo-location database is indispensable in the given respect, and I uploaded the source code for the necessary tools for FreeBSD to the present GitHub site.

### The local Geo-location database

The idea is to obtain the authoritative Geo-location information from the 5 RIR's, compile it into an optimized format suitable for quickly looking up the country codes of given IP addresses. This information is present in so called delegation statistics files on the ftp servers of each RIR, and APNIC, LACNIC and RIPENCC mirror these files of each of the other RIR's on their servers as well, while ARIN and AFRINIC do not mirror the latest delegation statistics of the other RIR's.

This GitHub repository provides the source code of the tools and a shell script `ipdb-update.sh` which can be used for the purpose. Download the package from GitHub to your FreeBSD system, and as user `root` execute `# make install` from within the `ipdb` directory.

Choose one of the three useful mirror sites, depending on where you are located:

-   Europe and Eurasia – RIPENCC:     [ftp.ripencc.net](ftp://ftp.ripencc.net) [default mirror]
-   Asia Pacific – APNIC:     [ftp.apnic.net](ftp://ftp.apnic.net)
-   Latin America and Caribbean – LACNIC:     [ftp.lacnic.net](ftp://ftp.lacnic.net)

As user `root` run the script `ipdb-update.sh` with the chosen mirror as the parameter, e.g. ftp.apnic.net:

    # ipdb-update.sh ftp.ripencc.net
    >>>>
    /usr/local/etc/ipdb/IPRanges/afrinic.md5      100% of   74  B  225 kBps 00m00s
    /usr/local/etc/ipdb/IPRanges/afrinic.dat      100% of  397 kB 1294 kBps 00m01s
    /usr/local/etc/ipdb/IPRanges/apnic.md5        100% of   73  B  230 kBps 00m00s
    /usr/local/etc/ipdb/IPRanges/apnic.dat        100% of 4023 kB 1252 kBps 00m03s
    /usr/local/etc/ipdb/IPRanges/arin.md5         100% of   67  B   77 kBps 00m00s
    /usr/local/etc/ipdb/IPRanges/arin.dat         100% of 8154 kB 1244 kBps 00m06s
    /usr/local/etc/ipdb/IPRanges/lacnic.md5       100% of   74  B  208 kBps 00m00s
    /usr/local/etc/ipdb/IPRanges/lacnic.dat       100% of 1861 kB 1263 kBps 00m02s
    /usr/local/etc/ipdb/IPRanges/ripencc.md5      100% of   74  B  284 kBps 00m00s
    /usr/local/etc/ipdb/IPRanges/ripencc.dat      100% of   10 MB 1246 kBps 00m09s
    ipdb v1.0 (24), Copyright © 2016 Dr. Rolf Jansen
    Processing RIR data files ...

     afrinic.dat  apnic.dat  arin.dat  lacnic.dat  ripencc.dat 

    Number of processed IPv4-Ranges = 83358

This will download all the delegation statistics data files together with the respective MD5 verification hashes into `/usr/local/etc/ipdb/IPRanges/`. This directory will be created if it does not exist. If the downloads went smooth, the script starts the `ipdb` tool in order to generate the binary file with the consolidated IPv4 ranges and country codes right in the same go. Later, you may want to put above `ipdb-update.sh` command into a weekly cron job.

Now, check whether the database is ready by looking up some IPv4 addresses using the `ipup` tool:

    $ ipup 62.175.157.33
    62.175.157.33 in 62.174.0.0-62.175.255.255 in ES

    $ ipup 141.33.17.2
    141.33.17.2 in 141.12.0.0-141.80.255.255 in DE

    $ ipup 99.67.80.80
    99.67.80.80 in 98.160.0.0-99.191.255.255 in US

    $ ipup 192.168.1.1
    192.168.1.1 not found

### Geo-blocking with ipfw

The `ipup` tool can generate tables of CIDR ranges for selected country codes which can be directly piped into ipfw, and with that the ipfw configuration script may contain something like:

    ...
    # allow only web access from DE, BR, US:
    /usr/local/bin/ipup -t DE:BR:US -n 7 | /sbin/ipfw -q /dev/stdin
    /sbin/ipfw -q add 70 deny tcp from not table\(7\) to any 80,443 in recv em0 setup
    ...

OR, the other way around:

    ...
    # deny web access from certain disgraceful regions:
    /usr/local/bin/v -t TR:SA:RU:GB -n 6 | /sbin/ipfw -q /dev/stdin
    /sbin/ipfw -q add 70 allow tcp from not table\(6\) to any 80,443 in recv em0 setup
    ...

### Is Geo-blocking evil?

As always, this depends solely on what's the purpose. Geo-blocking can hardly be held evil if you want to reduce the attack surface to your home server by limiting access to source IP addresses from the country where you live. Now, if you block access from the free (TR, SA), the freer (RU) and the freest (GB) countries in the world, this may be held evil in those countries :-D
