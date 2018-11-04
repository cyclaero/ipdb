//  ipdb.c
//  ipdb
//
//  Created by Dr. Rolf Jansen on 2016-07-10.
//  Copyright © 2016-2018 Dr. Rolf Jansen. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
//  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
//  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
//  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
//  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "utils.h"
#include "uint128t.h"
#include "store.h"


IP4Node *IP4Store = NULL;
IP6Node *IP6Store = NULL;
IP4Node *NS4Store = NULL;
IP6Node *NS6Store = NULL;

boolean readRIRStatisticsFormat_v2(FILE *in, size_t totalsize, int *ip_count, int *ns_count)
{
   boolean rc = false;

   size_t chunksize = (totalsize <= 67108864) ? totalsize : 67108864;   // allow max. allocation of 64 MB
   size_t totalread = 0;
   size_t bytesread, offset = 0;

   char *data = allocate(chunksize+1, default_align, false);
   char  ver[4] = {};
   char  reg[8] = {};

   while (totalread < totalsize && (bytesread = fread(data+offset, 1, chunksize-offset, in)) > 0)
   {
      totalread += bytesread;
      bytesread += offset;
      data[bytesread] = '\0';

      int   vl = 0, rl = 0, fl, ll;
      char *line = data;
      char *nextline, *cc, *cnt, *pfx, *ns, *iv, *ip;

      while (line < data + bytesread)
      {
         ll = linelen(line);
         nextline = line + ll;

         if (nextline[0] != '\0' && nextline[1] != '\0')
            *nextline++ = '\0';
         else
         {
            offset = strmlcpy(data, line, 0, NULL);
            break;
         }

         if (*line && *line != '#')
         {
            if (!vl)                            // has the data format version been read?
            {
               if (*line != '2')
                  goto quit;                    // only version 2[.x] is supported

               vl = fieldlen(line);
               strmlcpy(ver, line, 4, &vl);
               line += vl+1;

               rl = fieldlen(line);
               strmlcpy(reg, line, 8, &rl);
               rl++;
            }

            else if (*(cc = line+rl) != '*')    // skip the summary lines
            {
               fl = fieldlen(cc);
               iv = cc+fl+1;                    // the ip version
               if (fl)
                  if (cmp4(iv, "ipv4"))
                  {
                     IP4Node *node;
                     uint32_t ipst, ipct, iplo, iphi;

                     uppercase(cc, fl);
                     cc[fl] = '\0';
                     if (*cc)
                     {
                        ip = iv+fieldlen(iv)+1;
                        fl = fieldlen(ip);
                        ip[fl] = '\0';
                        cnt = ip+fl+1;
                        if ((ipst = ipv4_str2bin(ip))
                         && (ipct = (uint32_t)strtoul(cnt, NULL, 10)))
                        {
                           if (!cmp2(cc, "EU"))
                           {
                              iplo = ipst, iphi = iplo + ipct - 1;
                              while (node = findNet4Node(iplo, iphi, *(uint16_t*)cc, NULL, IP4Store))
                              {
                                 if (node->lo < iplo)
                                    iplo = node->lo;

                                 if (node->hi > iphi)
                                    iphi = node->hi;

                                 removeIP4Node(node->lo, &IP4Store); (*ip_count)--;
                              }

                              addIP4Node(iplo, iphi, *(uint16_t*)cc, NULL, &IP4Store); (*ip_count)++;
                           }

                           ns = cnt + fieldlen(cnt) + 1; // timestamp, f.ex.: 20120605
                           ns += fieldlen(ns) + 1;       // status: assigned, allocated, available, reserved
                           ns += fieldlen(ns) + 1;       // unique identifier of the ASN owner, f.ex.: 5a5f320b-aefc-4f38-8b03-dff796ea678d
                           ns[fieldlen(ns)] = '\0';
                           iplo = ipst, iphi = iplo + ipct - 1;
                           while (node = findNet4Node(iplo, iphi, 0, ns, NS4Store))
                           {
                              if (node->lo < iplo)
                                 iplo = node->lo;

                              if (node->hi > iphi)
                                 iphi = node->hi;

                              removeIP4Node(node->lo, &NS4Store); (*ns_count)--;
                           }

                           addIP4Node(iplo, iphi, 0, ns, &NS4Store); (*ns_count)++;
                        }
                     }
                  }

                  else if (cmp4(iv, "ipv6"))
                  {
                     IP6Node *node;
                     int32_t  ipfx;
                     uint128t ipst, iplo, iphi;

                     uppercase(cc, fl);
                     cc[fl] = '\0';
                     if (*cc)
                     {
                        ip = iv+fieldlen(iv)+1;
                        fl = fieldlen(ip);
                        ip[fl] = '\0';
                        pfx = ip+fl+1;
                        if (gt_u128(ipst = ipv6_str2bin(ip), u64_to_u128t(0))
                         && (ipfx = 128 - (int32_t)strtoul(pfx, NULL, 10)) >= 0)
                        {
                           if (!cmp2(cc, "EU"))
                           {
                              iplo = ipst, iphi = add_u128(ipst, inteb6_m1(ipfx));
                              while (node = findNet6Node(iplo, iphi, *(uint16_t*)cc, NULL, IP6Store))
                              {
                                 if (lt_u128(node->lo, iplo))
                                    iplo = node->lo;

                                 if (gt_u128(node->hi, iphi))
                                    iphi = node->hi;

                                 removeIP6Node(node->lo, &IP6Store); (*ip_count)--;
                              }

                              addIP6Node(iplo, iphi, *(uint16_t*)cc, NULL, &IP6Store); (*ip_count)++;
                           }

                           ns = pfx + fieldlen(pfx) + 1; // timestamp, f.ex.: 20120605
                           ns += fieldlen(ns) + 1;       // status: assigned, allocated, available, reserved
                           ns += fieldlen(ns) + 1;       // unique identifier of the ASN owner, f.ex.: 5a5f320b-aefc-4f38-8b03-dff796ea678d
                           ns[fieldlen(ns)] = '\0';
                           iplo = ipst, iphi = add_u128(ipst, inteb6_m1(ipfx));
                           while (node = findNet6Node(iplo, iphi, 0, ns, NS6Store))
                           {
                              if (lt_u128(node->lo, iplo))
                                 iplo = node->lo;

                              if (gt_u128(node->hi, iphi))
                                 iphi = node->hi;

                              removeIP6Node(node->lo, &NS6Store); (*ns_count)--;
                           }

                           addIP6Node(iplo, iphi, 0, ns, &NS6Store); (*ns_count)++;
                        }
                     }
                  }
            }
         }

         line = nextline;
      }
   }

   rc = (*ip_count + *ns_count != 0);

quit:
   deallocate(VPR(data), false);
   return rc;
}


int main(int argc, const char *argv[])
{
   if (argc >= 3)
   {
      int   namelen = strvlen(argv[1]);
      char *outIP4Name = strcpy(alloca(OSP(namelen+4)), argv[1]); cpy4(outIP4Name+namelen, ".v4");
      char *outIP6Name = strcpy(alloca(OSP(namelen+4)), argv[1]); cpy4(outIP6Name+namelen, ".v6");
      char *outNS4Name = strcpy(alloca(OSP(namelen+4)), argv[1]); cpy4(outNS4Name+namelen, ".s4");
      char *outNS6Name = strcpy(alloca(OSP(namelen+4)), argv[1]); cpy4(outNS6Name+namelen, ".s6");
      FILE *outIP4, *outIP6, *outNS4, *outNS6;

      if (outIP4 = fopen(outIP4Name, "w"))
         if (outIP6 = fopen(outIP6Name, "w"))
            if (outNS4 = fopen(outNS4Name, "w"))
               if (outNS6 = fopen(outNS6Name, "w"))
               {
                  int ip_count, ns_count, ip_total = 0, ns_total = 0;

                  FILE  *in;
                  struct stat st;

                  printf("ipdb v1.2b (" SCMREV "), Copyright © 2016-2018 Dr. Rolf Jansen\nProcessing RIR data files ...\n\n");
                  for (int inc = 2; inc < argc; inc++)
                  {
                     if (stat(argv[inc], &st) == no_error && st.st_size && (in = fopen(argv[inc], "r")))
                     {
                        const char *file = strrchr(argv[inc], '/');
                        if (file)
                           file++;
                        else
                           file = argv[inc];
                        printf(" %s ", file);
                        fflush(stdout);

                        ip_count = ns_count = 0;
                        if (readRIRStatisticsFormat_v2(in, (size_t)st.st_size, &ip_count, &ns_count))
                           ip_total += ip_count, ns_total += ns_count;

                        fclose(in);
                     }

                     else
                     {
                        fclose(outNS6), fclose(outNS4), fclose(outIP6), fclose(outIP4);
                        printf("\n");
                        return 1;
                     }
                  }

                  serializeIP4Tree(outIP4, IP4Store);
                  releaseIP4Tree(IP4Store);

                  serializeIP6Tree(outIP6, IP6Store);
                  releaseIP6Tree(IP6Store);

                  serializeIP4Tree(outNS4, NS4Store);
                  releaseIP4Tree(NS4Store);

                  serializeIP6Tree(outNS6, NS6Store);
                  releaseIP6Tree(NS6Store);

                  fclose(outNS6), fclose(outNS4), fclose(outIP6), fclose(outIP4);

                  printf("\n\nTotal number of processed IP-Ranges = %d\nTotal number of processed Segments  = %d\n", ip_total, ns_total);
                  return 0;
               }
               else
                  fclose(outNS4), fclose(outIP6), fclose(outIP4);
         else
            fclose(outIP6), fclose(outIP4);
      else
         fclose(outIP4);
   }

   return 1;
}
