//  store.h
//  ipdbtools
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


#pragma mark ••• AVL Tree of IPv4-Ranges •••

typedef union
{
   uint8_t   byte[4];
   uint32_t  number;
} IP4Desc;

typedef uint32_t IP4Set[3];   // [0] -> lo, [1] -> hi, [2] -> cc

typedef struct IP4Node
{
   uint32_t lo, hi;           // IPv4 number range
   uint32_t cc;               // country code

   int32_t B;                 // house holding
   struct IP4Node *L, *R;
} IP4Node;

IP4Node  *findIP4Node(uint32_t ip, IP4Node  *node);
IP4Node *findNet4Node(uint32_t lo, uint32_t hi, uint32_t cc, IP4Node  *node);
int        addIP4Node(uint32_t lo, uint32_t hi, uint32_t cc, IP4Node **node);
int     removeIP4Node(uint32_t ip, IP4Node **node);
void serializeIP4Tree(FILE *out, IP4Node *node);
void   releaseIP4Tree(IP4Node *node);

static inline int bisectionIP4Search(uint32_t ip4, IP4Set *sortedIP4Sets, int count)
{
   uint32_t  u;
   int o, p, q;
   for (p = 0, q = count-1; p <= q;)
   {
      o = (p + q) >> 1;
      if ((u = sortedIP4Sets[o][0]) <= ip4 && ip4 <= sortedIP4Sets[o][1])
         return o;
      else if (ip4 < u)
         q = o-1;
      else // (ip4 > sortedIP4Sets[o][1])
         p = o+1;
   }

   return -1;
}


#pragma mark ••• AVL Tree of IPv6-Ranges •••

typedef union
{
   uint64_t quad[2];
   uint16_t word[8];
   uint128t number;
} IP6Desc;

typedef uint128t IP6Set[3];  // [0] -> lo, [1] -> hi, [2] -> cc

typedef struct IP6Node
{
   uint128t lo, hi;          // IPv4 number range
   uint32_t cc;              // country code

   int32_t B;                 // house holding
   struct IP6Node *L, *R;
} IP6Node;

IP6Node  *findIP6Node(uint128t ip, IP6Node *node);
IP6Node *findNet6Node(uint128t lo, uint128t hi, uint32_t cc, IP6Node  *node);
int        addIP6Node(uint128t lo, uint128t hi, uint32_t cc, IP6Node **node);
int     removeIP6Node(uint128t ip, IP6Node **node);
void serializeIP6Tree(FILE *out, IP6Node *node);
void   releaseIP6Tree(IP6Node *node);

static inline int bisectionIP6Search(uint128t ip6, IP6Set *sortedIP6Sets, int count)
{
   uint128t u;
   int o, p, q;
   for (p = 0, q = count-1; p <= q;)
   {
      o = (p + q) >> 1;
      if (le_u128(u = sortedIP6Sets[o][0], ip6)  && le_u128(ip6, sortedIP6Sets[o][1]))
         return o;
      else if (lt_u128(ip6, u))
         q = o-1;
      else // (gt_u128(ip6, sortedIP6Sets[o][1]))
         p = o+1;
   }

   return -1;
}


#pragma mark ••• AVL Tree of Country Codes •••

typedef struct CCNode
{
   uint32_t cc;            // country code
   uint32_t ui;            // user info

   int32_t  B;             // house holding
   struct CCNode *L, *R;
} CCNode;

CCNode *findCCNode(uint32_t cc, CCNode  *node);
int      addCCNode(uint32_t cc, uint32_t ui, CCNode **node);
int   removeCCNode(uint32_t cc, CCNode **node);
void releaseCCTree(CCNode *node);


#pragma mark ••• Pseudo Hash Table of Country Codes •••

#define ccTableSize 676

static inline uint32_t cce(uint16_t cc)
{
   uint8_t *ca = (uint8_t *)&cc;
   return (ca[b2_0]-'A')*26 + (ca[b2_1]-'A');   // AA to ZZ ranges from 0 to 675
}

CCNode **createCCTable(void);
void    releaseCCTable(CCNode *table[]);

CCNode *findCC(CCNode *table[], uint32_t cc);
void   storeCC(CCNode *table[], char *ccui);
void  removeCC(CCNode *table[], uint32_t cc);



#pragma mark ••• IP number/string utility functions •••

#include <sys/socket.h>
#include <arpa/inet.h>

static inline uint32_t ipv4_str2bin(char *str)
{
   uint32_t bin;
   return (inet_pton(AF_INET, str, &bin) > 0)
          ? SwapInt32(bin)
          : 0;
}

typedef char IP4Str[16];
static inline char *ipv4_bin2str(uint32_t bin, char *str)
{
   IP4Desc ipdsc = {.number = bin};
   sprintf(str, "%d.%d.%d.%d", ipdsc.byte[b4_3], ipdsc.byte[b4_2], ipdsc.byte[b4_1], ipdsc.byte[b4_0]);
   return str;
}

static inline uint128t ipv6_str2bin(char *str)
{
   uint64_t bin[2];
   return (inet_pton(AF_INET6, str, &bin) > 0)
          ? (IP6Desc){SwapInt64(bin[b2_1]), SwapInt64(bin[b2_0])}.number
          : u64_to_u128t(0);
}

typedef char IP6Str[40];
static inline char *ipv6_bin2str(uint128t bin, char *str)
{
   IP6Desc ipdsc = {.number = bin};
   sprintf(str, "%x:%x:%x:%x:%x:%x:%x:%x", ipdsc.word[b8_7], ipdsc.word[b8_6], ipdsc.word[b8_5], ipdsc.word[b8_4],
                                           ipdsc.word[b8_3], ipdsc.word[b8_2], ipdsc.word[b8_1], ipdsc.word[b8_0]);
   return str;
}


static inline int32_t intlb4_1p(double v)
{
   return (int32_t)log2(v+1);
}


static const IP6Desc v6[129] =
{
   {0x0, 0x0}, {0x1, 0x0}, {0x3, 0x0}, {0x7, 0x0}, {0xF, 0x0}, {0x1F, 0x0}, {0x3F, 0x0}, {0x7F, 0x0}, {0xFF, 0x0},
   {0x1FF, 0x0}, {0x3FF, 0x0}, {0x7FF, 0x0}, {0xFFF, 0x0}, {0x1FFF, 0x0}, {0x3FFF, 0x0}, {0x7FFF, 0x0}, {0xFFFF, 0x0},
   {0x1FFFF, 0x0}, {0x3FFFF, 0x0}, {0x7FFFF, 0x0}, {0xFFFFF, 0x0}, {0x1FFFFF, 0x0}, {0x3FFFFF, 0x0}, {0x7FFFFF, 0x0},
   {0xFFFFFF, 0x0}, {0x1FFFFFF, 0x0}, {0x3FFFFFF, 0x0}, {0x7FFFFFF, 0x0}, {0xFFFFFFF, 0x0}, {0x1FFFFFFF, 0x0}, {0x3FFFFFFF, 0x0},
   {0x7FFFFFFF, 0x0}, {0xFFFFFFFF, 0x0}, {0x1FFFFFFFF, 0x0}, {0x3FFFFFFFF, 0x0}, {0x7FFFFFFFF, 0x0}, {0xFFFFFFFFF, 0x0},
   {0x1FFFFFFFFF, 0x0}, {0x3FFFFFFFFF, 0x0}, {0x7FFFFFFFFF, 0x0}, {0xFFFFFFFFFF, 0x0}, {0x1FFFFFFFFFF, 0x0}, {0x3FFFFFFFFFF, 0x0},
   {0x7FFFFFFFFFF, 0x0}, {0xFFFFFFFFFFF, 0x0}, {0x1FFFFFFFFFFF, 0x0}, {0x3FFFFFFFFFFF, 0x0}, {0x7FFFFFFFFFFF, 0x0},
   {0xFFFFFFFFFFFF, 0x0}, {0x1FFFFFFFFFFFF, 0x0}, {0x3FFFFFFFFFFFF, 0x0}, {0x7FFFFFFFFFFFF, 0x0}, {0xFFFFFFFFFFFFF, 0x0},
   {0x1FFFFFFFFFFFFF, 0x0}, {0x3FFFFFFFFFFFFF, 0x0}, {0x7FFFFFFFFFFFFF, 0x0}, {0xFFFFFFFFFFFFFF, 0x0}, {0x1FFFFFFFFFFFFFF, 0x0},
   {0x3FFFFFFFFFFFFFF, 0x0}, {0x7FFFFFFFFFFFFFF, 0x0}, {0xFFFFFFFFFFFFFFF, 0x0}, {0x1FFFFFFFFFFFFFFF, 0x0}, {0x3FFFFFFFFFFFFFFF, 0x0},
   {0x7FFFFFFFFFFFFFFF, 0x0}, {0xFFFFFFFFFFFFFFFF, 0x0}, {0xFFFFFFFFFFFFFFFF, 0x1}, {0xFFFFFFFFFFFFFFFF, 0x3},
   {0xFFFFFFFFFFFFFFFF, 0x7}, {0xFFFFFFFFFFFFFFFF, 0xF}, {0xFFFFFFFFFFFFFFFF, 0x1F}, {0xFFFFFFFFFFFFFFFF, 0x3F},
   {0xFFFFFFFFFFFFFFFF, 0x7F}, {0xFFFFFFFFFFFFFFFF, 0xFF}, {0xFFFFFFFFFFFFFFFF, 0x1FF}, {0xFFFFFFFFFFFFFFFF, 0x3FF},
   {0xFFFFFFFFFFFFFFFF, 0x7FF}, {0xFFFFFFFFFFFFFFFF, 0xFFF}, {0xFFFFFFFFFFFFFFFF, 0x1FFF}, {0xFFFFFFFFFFFFFFFF, 0x3FFF},
   {0xFFFFFFFFFFFFFFFF, 0x7FFF}, {0xFFFFFFFFFFFFFFFF, 0xFFFF}, {0xFFFFFFFFFFFFFFFF, 0x1FFFF}, {0xFFFFFFFFFFFFFFFF, 0x3FFFF},
   {0xFFFFFFFFFFFFFFFF, 0x7FFFF}, {0xFFFFFFFFFFFFFFFF, 0xFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x1FFFFF}, {0xFFFFFFFFFFFFFFFF, 0x3FFFFF},
   {0xFFFFFFFFFFFFFFFF, 0x7FFFFF}, {0xFFFFFFFFFFFFFFFF, 0xFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x1FFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x3FFFFFF},
   {0xFFFFFFFFFFFFFFFF, 0x7FFFFFF}, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x1FFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x3FFFFFFF},
   {0xFFFFFFFFFFFFFFFF, 0x7FFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x1FFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x3FFFFFFFF},
   {0xFFFFFFFFFFFFFFFF, 0x7FFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x1FFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x3FFFFFFFFF},
   {0xFFFFFFFFFFFFFFFF, 0x7FFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x1FFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x3FFFFFFFFFF},
   {0xFFFFFFFFFFFFFFFF, 0x7FFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x1FFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x3FFFFFFFFFFF},
   {0xFFFFFFFFFFFFFFFF, 0x7FFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x1FFFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x3FFFFFFFFFFFF},
   {0xFFFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x1FFFFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x3FFFFFFFFFFFFF},
   {0xFFFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x1FFFFFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x3FFFFFFFFFFFFFF},
   {0xFFFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x1FFFFFFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0x3FFFFFFFFFFFFFFF},
   {0xFFFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF}, {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF}
};

static inline uint128t inteb6_m1(int32_t e)
{
   return (0 <= e && e <= 127) ? v6[e].number : u64_to_u128t(1);
}

static inline int32_t intlb6_1p(uint128t v)
{
   uint128t u;
   int o, p, q;
   for (p = 0, q = 127; p <= q;)
   {
      o = (p + q) >> 1;
      if (le_u128(u = v6[o].number, v) && lt_u128(v, v6[o+1].number))
         return o;
      else if (lt_u128(v, u))
         q = o-1;
      else // (ge_u128(v, v6[o+1]))
         p = o+1;
   }

   return 0;
}
