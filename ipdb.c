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
#include <string.h>
#include <tmmintrin.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "store.h"


IPNode *IPStore = NULL;

int readRIPEDataBaseFormat(FILE *in, size_t totalsize)
{
   int count = 0;
   IPNode  *node;
   IPv4Desc ipdsc_lo, ipdsc_hi;

   size_t chunksize = (totalsize <= 67108864) ? totalsize : 67108864;   // allow max. allocation of 64 MB
   size_t totalread = 0;
   size_t bytesread, offset = 0;

   char *data = allocate(chunksize+1, false);

   while (totalread < totalsize && (bytesread = fread(data+offset, 1, chunksize-offset, in)) > 0)
   {
      totalread += bytesread;
      bytesread += offset;
      data[bytesread] = '\0';

      int   ll;
      char *line = data;
      char *nextline, *ip = NULL, *cc = NULL;

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

         if (*(uint64_t*)line == *(uint64_t*)"inetnum:")
         {
            line += 8, ip = line + blanklen(line);
            if (*ip)
            {
               char *ipstr_lo = ip;
               ip += wordlen(ip);
               *ip++ = '\0';
               while (*ip && (*ip == '-' || *ip <= ' '))
                 ip++;
               char *ipstr_hi = ip;

               sscanf(ipstr_lo, "%hhu.%hhu.%hhu.%hhu", &ipdsc_lo.nibble[3], &ipdsc_lo.nibble[2], &ipdsc_lo.nibble[1], &ipdsc_lo.nibble[0]);
               if (*ipstr_hi)
                  sscanf(ipstr_hi, "%hhu.%hhu.%hhu.%hhu", &ipdsc_hi.nibble[3], &ipdsc_hi.nibble[2], &ipdsc_hi.nibble[1], &ipdsc_hi.nibble[0]);
               else
                  ipdsc_hi.number = 0;
            }
         }

         if (*(uint64_t*)line == *(uint64_t*)"country:")
         {
            line += 8, cc = line + blanklen(line);
            if (ipdsc_lo.number && ipdsc_hi.number && *cc)
            {
               uppercase(cc, 2);
               if (*(uint16_t*)cc != *(uint16_t*)"EU")
               {
                  while (node = findNetNode(ipdsc_lo.number, ipdsc_hi.number, *(uint16_t*)cc, IPStore))
                  {
                     if (node->lo < ipdsc_lo.number)
                        ipdsc_lo.number = node->lo;

                     if (node->hi > ipdsc_hi.number)
                        ipdsc_hi.number = node->hi;

                     removeIPNode(node->lo, &IPStore); count--;
                  }

                  addIPNode(ipdsc_lo.number, ipdsc_hi.number, *(uint16_t*)cc, &IPStore); count++;
               }
            }

            ipdsc_lo.number = ipdsc_hi.number = 0;
            cc = NULL;
         }

         line = nextline;
      }
   }

   deallocate(VPR(data), false);
   return count;
}


int readRIRStatisticsFormat_v2(FILE *in, size_t totalsize)
{
   int count = 0;

   size_t chunksize = (totalsize <= 67108864) ? totalsize : 67108864;   // allow max. allocation of 64 MB
   size_t totalread = 0;
   size_t bytesread, offset = 0;

   char *data = allocate(chunksize+1, false);
   char  ver[4] = {};
   char  reg[8] = {};

   IPNode  *node;
   IPv4Desc ipdsc;

   while (totalread < totalsize && (bytesread = fread(data+offset, 1, chunksize-offset, in)) > 0)
   {
      totalread += bytesread;
      bytesread += offset;
      data[bytesread] = '\0';

      int   vl = 0, rl = 0, fl, ll;
      char *line = data;
      char *nextline, *cc, *iv, *ip, *ct;
      uint32_t iplo, iphi;

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
               if (fl && *(uint32_t*)iv == *(uint32_t*)"ipv4")
               {
                  uppercase(cc, fl);
                  cc[fl] = '\0';
                  if (*(uint16_t*)cc != *(uint16_t*)"EU")
                  {
                     ip = iv+fieldlen(iv)+1;
                     fl = fieldlen(ip);
                     ip[fl] = '\0';
                     sscanf(ip, "%hhu.%hhu.%hhu.%hhu", &ipdsc.nibble[3], &ipdsc.nibble[2], &ipdsc.nibble[1], &ipdsc.nibble[0]);
                     iplo = ipdsc.number;

                     ct = ip+fl+1;
                     iphi = iplo + (uint32_t)strtoul(ct, NULL, 10) - 1;

                     while (node = findNetNode(iplo, iphi, *(uint16_t*)cc, IPStore))
                     {
                        if (node->lo < iplo)
                           iplo = node->lo;

                        if (node->hi > iphi)
                           iphi = node->hi;

                        removeIPNode(node->lo, &IPStore); count--;
                     }

                     addIPNode(iplo, iphi, *(uint16_t*)cc, &IPStore); count++;
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
      int count = 0;
      struct stat st;
      FILE *in, *out;

      if (out = fopen(argv[1], "w"))
      {
         printf("ipdb v1.0 ("SVNREV"), Copyright © 2016 Dr. Rolf Jansen\nProcessing RIR data files ...\n\n");
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
               if (strcmp(file, "ripe.db") != 0)
                  count += readRIRStatisticsFormat_v2(in, st.st_size);
               else
                  count += readRIPEDataBaseFormat(in, st.st_size);

               fclose(in);
            }

            else
            {
               fclose(out);
               printf("\n");
               return 1;
            }
         }

         serializeIPTree(out, IPStore);
         releaseIPTree(IPStore);
         fclose(out);

         printf("\n\nNumber of processed IPv4-Ranges = %d\n", count);
         return 0;
      }
   }

   return 1;
}
