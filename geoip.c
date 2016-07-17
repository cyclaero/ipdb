//  geoip.c
//  geoip
//
//  Created by Dr. Rolf Jansen on 2016-07-17.
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
#include <unistd.h>
#include <tmmintrin.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "store.h"


void usage(const char *executable)
{
   const char *r = executable + strvlen(executable);
   while (--r >= executable && *r != '/'); r++;
   printf("%s v1.0 ("SVNREV"), Copyright © 2016 Dr. Rolf Jansen\n", r);
   printf("usage:  [-r bstfile] [-h] <dotted IPv4 address>\n");
   printf(" -r bstfile    the path to the binary file with the consolidated IP ranges that has been.\n");
   printf("               generated by the 'ipdb' tool [default: /usr/local/etc/ipdb/IPRanges/ipcc.bst].\n");
   printf(" -h            show these usage instructions.\n");
   printf(" <IP4 address> the dotted IPv4 address to be looked-up.\n\n");
}


IPNode *IPStore = NULL;

int main(int argc, char *argv[])
{
   int   ch;
   char *cmd      = argv[0];
   char *bstfname = "/usr/local/etc/ipdb/IPRanges/ipcc.bst";

   while ((ch = getopt(argc, argv, "r:h")) != -1)
   {
      switch (ch)
      {
         case 'r':
            bstfname = optarg;
            break;

         case 'h':
         default:
            usage(cmd);
            return 1;
      }
   }

   argc -= optind;
   argv += optind;

   if (argc != 1)
   {
      usage(cmd);
      return 1;
   }

   struct stat st;
   FILE *in;
   if (stat(bstfname, &st) == noerr && st.st_size && (in = fopen(bstfname, "r")))
   {
      IPSet *sortedIPSets = allocate(st.st_size, false);
      if (fread(sortedIPSets, st.st_size, 1, in))
         IPStore = sortedIPSetsToTree(sortedIPSets, 0, (int)(st.st_size/sizeof(uint32_t))/3 - 1);
      deallocate(VPR(sortedIPSets), false);
      fclose(in);

      IPv4Desc ipdsc,ipdsc_lo, ipdsc_hi;
      sscanf(argv[0], "%hhu.%hhu.%hhu.%hhu", &ipdsc.nibble[3], &ipdsc.nibble[2], &ipdsc.nibble[1], &ipdsc.nibble[0]);

      IPNode *node = findIPNode(ipdsc.number, IPStore);
      if (node)
      {
         ipdsc_lo.number = node->lo;
         ipdsc_hi.number = node->hi;
         printf("%s in %d.%d.%d.%d-%d.%d.%d.%d in %s\n\n", argv[0],
                                                          ipdsc_lo.nibble[3], ipdsc_lo.nibble[2], ipdsc_lo.nibble[1], ipdsc_lo.nibble[0],
                                                          ipdsc_hi.nibble[3], ipdsc_hi.nibble[2], ipdsc_hi.nibble[1], ipdsc_hi.nibble[0],
                                                          (char *)&node->cc);
      }
      else
         printf("%s not found\n\n", argv[0]);

      releaseIPTree(IPStore);
      return 0;
   }

   return 1;
}
