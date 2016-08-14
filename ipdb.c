//  ipdb.c
//  ipdb
//
//  Created by Dr. Rolf Jansen on 2016-07-10.
//  Copyright © 2016 projectworld.net. All rights reserved.
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
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "binutils.h"
#include "store.h"


IP4Node *IP4Store = NULL;
IP6Node *IP6Store = NULL;

int readRIRStatisticsFormat_v2(FILE *in, size_t totalsize)
{
   int count = 0;

   size_t chunksize = (totalsize <= 67108864) ? totalsize : 67108864;   // allow max. allocation of 64 MB
   size_t totalread = 0;
   size_t bytesread, offset = 0;

   char *data = allocate(chunksize+1, false);
   char  ver[4] = {};
   char  reg[8] = {};

   while (totalread < totalsize && (bytesread = fread(data+offset, 1, chunksize-offset, in)) > 0)
   {
      totalread += bytesread;
      bytesread += offset;
      data[bytesread] = '\0';

      int   vl = 0, rl = 0, fl, ll;
      char *line = data;
      char *nextline, *cc, *iv, *ip;

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
               {
                  count = -1;
                  goto quit;                    // only version 2[.x] is supported
               }

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
                  if (*(uint32_t*)iv == *(uint32_t*)"ipv4")
                  {
                     IP4Node *node;
                     uint32_t iplo, iphi;

                     uppercase(cc, fl);
                     cc[fl] = '\0';
                     if (*(uint16_t*)cc != *(uint16_t*)"EU")
                     {
                        ip = iv+fieldlen(iv)+1;
                        fl = fieldlen(ip);
                        ip[fl] = '\0';
                        if (iplo = ipv4_str2bin(ip))
                        {
                           char *ct = ip+fl+1;
                           iphi = iplo + (uint32_t)strtoul(ct, NULL, 10) - 1;

                           while (node = findNet4Node(iplo, iphi, *(uint16_t*)cc, IP4Store))
                           {
                              if (node->lo < iplo)
                                 iplo = node->lo;

                              if (node->hi > iphi)
                                 iphi = node->hi;

                              removeIP4Node(node->lo, &IP4Store); count--;
                           }

                           addIP4Node(iplo, iphi, *(uint16_t*)cc, &IP4Store); count++;
                        }
                     }
                  }

                  else if (*(uint32_t*)iv == *(uint32_t*)"ipv6")
                  {
                     IP6Node *node;
                     uint128t iplo, iphi;

                     uppercase(cc, fl);
                     cc[fl] = '\0';
                     if (*(uint16_t*)cc != *(uint16_t*)"EU")
                     {
                        ip = iv+fieldlen(iv)+1;
                        fl = fieldlen(ip);
                        ip[fl] = '\0';
                        if (gt_u128(iplo = ipv6_str2bin(ip), u64_to_u128t(0)))
                        {
                           char *pfx = ip+fl+1;
                           iphi = add_u128(iplo, inteb6_m1(128 - (int32_t)strtoul(pfx, NULL, 10)));

                           while (node = findNet6Node(iplo, iphi, *(uint16_t*)cc, IP6Store))
                           {
                              if (lt_u128(node->lo, iplo))
                                 iplo = node->lo;

                              if (gt_u128(node->hi, iphi))
                                 iphi = node->hi;

                              removeIP6Node(node->lo, &IP6Store); count--;
                           }

                           addIP6Node(iplo, iphi, *(uint16_t*)cc, &IP6Store); count++;
                        }
                     }
                  }
            }
         }

         line = nextline;
      }
   }

quit:
   deallocate(VPR(data), false);
   return count;
}


int main(int argc, const char *argv[])
{
   if (argc >= 3)
   {
      int   namelen = strvlen(argv[1]);
      char *out4Name = strcpy(alloca(namelen+4), argv[1]); *(uint32_t *)&out4Name[namelen] = *(uint32_t *)".v4";
      char *out6Name = strcpy(alloca(namelen+4), argv[1]); *(uint32_t *)&out6Name[namelen] = *(uint32_t *)".v6";
      FILE *out4, *out6;

      if (out4 = fopen(out4Name, "w"))
         if (out6 = fopen(out6Name, "w"))
         {
            int    count = 0;
            FILE  *in;
            struct stat st;

            printf("ipdb v1.1.1 ("SVNREV"), Copyright © 2016 Dr. Rolf Jansen\nProcessing RIR data files ...\n\n");
            for (int inc = 2; inc < argc; inc++)
            {
               if (stat(argv[inc], &st) == noerr && st.st_size && (in = fopen(argv[inc], "r")))
               {
                  const char *file = strrchr(argv[inc], '/');
                  if (file)
                     file++;
                  else
                     file = argv[inc];
                  printf(" %s ", file);
                  fflush(stdout);

                  count += readRIRStatisticsFormat_v2(in, (size_t)st.st_size);

                  fclose(in);
               }

               else
               {
                  fclose(out6);
                  fclose(out4);
                  printf("\n");
                  return 1;
               }
            }

            serializeIP4Tree(out4, IP4Store);
            releaseIP4Tree(IP4Store);

            serializeIP6Tree(out6, IP6Store);
            releaseIP6Tree(IP6Store);

            fclose(out6);
            fclose(out4);

            printf("\n\nNumber of processed IP-Ranges = %d\n", count);
            return 0;
         }
         else
            fclose(out4);
   }

   return 1;
}
