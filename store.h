//  store.h
//  ipdb / geoip / geod
//
//  Created by Dr. Rolf Jansen on 2016-07-10.
//  Copyright (c) 2016 projectworld.net. All rights reserved.
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


#define noerr 0

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

   #define b2_0 0
   #define b2_1 1

   #define b4_0 0
   #define b4_1 1
   #define b4_2 2
   #define b4_3 3

   #define b8_0 0
   #define b8_1 1
   #define b8_2 2
   #define b8_3 3
   #define b8_4 4
   #define b8_5 5
   #define b8_6 6
   #define b8_7 7

   #define swapInt16(x) _swapInt16(x)
   #define swapInt32(x) _swapInt32(x)
   #define swapInt64(x) _swapInt64(x)

   #if defined(__i386__) || defined(__x86_64__)

      static inline uint16_t _swapInt16(uint16_t x)
      {
         __asm__("rolw $8,%0" : "+q" (x));
         return x;
      }

      static inline uint32_t _swapInt32(uint32_t x)
      {
         __asm__("bswapl %0" : "+q" (x));
         return x;
      }

   #else

      static inline uint16_t _swapInt16(uint16_t x)
      {
         uint16_t z;
         uint8_t *p = (uint8_t *)&x;
         uint8_t *q = (uint8_t *)&z;
         q[0] = p[1];
         q[1] = p[0];
         return z;
      }

      static inline uint32_t _swapInt32(uint32_t x)
      {
         uint32_t z;
         uint8_t *p = (uint8_t *)&x;
         uint8_t *q = (uint8_t *)&z;
         q[0] = p[3];
         q[1] = p[2];
         q[2] = p[1];
         q[3] = p[0];
         return z;
      }

   #endif

   #if defined(__x86_64__)

      static inline uint64_t _swapInt64(uint64_t x)
      {
         __asm__("bswapq %0" : "+q" (x));
         return x;
      }

   #else

      static inline uint64_t _swapInt64(uint64_t x)
      {
         uint64_t z;
         uint8_t *p = (uint8_t *)&x;
         uint8_t *q = (uint8_t *)&z;
         q[0] = p[7];
         q[1] = p[6];
         q[2] = p[5];
         q[3] = p[4];
         q[4] = p[3];
         q[5] = p[2];
         q[6] = p[1];
         q[7] = p[0];
         return z;
      }

   #endif

#else

   #define b2_0 1
   #define b2_1 0

   #define b4_0 3
   #define b4_1 2
   #define b4_2 1
   #define b4_3 0

   #define b8_0 7
   #define b8_1 6
   #define b8_2 5
   #define b8_3 4
   #define b8_4 3
   #define b8_5 2
   #define b8_6 1
   #define b8_7 0

   #define swapInt16(x) (x)
   #define swapInt32(x) (x)
   #define swapInt64(x) (x)

#endif


#if defined(__x86_64__)

   #include <tmmintrin.h>

   static const __m128i nul16 = {0x0000000000000000ULL, 0x0000000000000000ULL};  // 16 bytes with nul
   static const __m128i lfd16 = {0x0A0A0A0A0A0A0A0AULL, 0x0A0A0A0A0A0A0A0AULL};  // 16 bytes with line feed
   static const __m128i col16 = {0x3A3A3A3A3A3A3A3AULL, 0x3A3A3A3A3A3A3A3AULL};  // 16 bytes with colon ':' limit
   static const __m128i vtl16 = {0x7C7C7C7C7C7C7C7CULL, 0x7C7C7C7C7C7C7C7CULL};  // 16 bytes with vertical line '|' limit
   static const __m128i blk16 = {0x2020202020202020ULL, 0x2020202020202020ULL};  // 16 bytes with inner blank limit
   static const __m128i obl16 = {0x2121212121212121ULL, 0x2121212121212121ULL};  // 16 bytes with outer blank limit

   // Drop-in replacement for strlen(), utilizing some builtin SSSE3 instructions
   static inline int strvlen(const char *str)
   {
      if (!*str)
         return 0;

      unsigned bmask;

      if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_lddqu_si128((__m128i *)str), nul16)))
         return __builtin_ctz(bmask);

      for (int len = 16 - (intptr_t)str%16;; len += 16)
         if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&str[len]), nul16)))
            return len + __builtin_ctz(bmask);
   }

   static inline int linelen(const char *line)
   {
      if (!*line)
         return 0;

      unsigned bmask;

      if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_lddqu_si128((__m128i *)line), nul16))
                | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_lddqu_si128((__m128i *)line), lfd16)))
         return __builtin_ctz(bmask);

      for (int len = 16 - (intptr_t)line%16;; len += 16)
         if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&line[len]), nul16))
                   | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&line[len]), lfd16)))
            return len + __builtin_ctz(bmask);
   }

   static inline int taglen(const char *tag)
   {
      if (!*tag)
         return 0;

      unsigned bmask;

      if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_lddqu_si128((__m128i *)tag), nul16))
                | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_lddqu_si128((__m128i *)tag), col16)))
         return __builtin_ctz(bmask);

      for (int len = 16 - (intptr_t)tag%16;; len += 16)
         if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&tag[len]), nul16))
                   | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&tag[len]), col16)))
            return len + __builtin_ctz(bmask);
   }

   static inline int fieldlen(const char *field)
   {
      if (!*field)
         return 0;

      unsigned bmask;

      if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_lddqu_si128((__m128i *)field), nul16))
                | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_lddqu_si128((__m128i *)field), vtl16)))
         return __builtin_ctz(bmask);

      for (int len = 16 - (intptr_t)field%16;; len += 16)
         if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&field[len]), nul16))
                   | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&field[len]), vtl16)))
            return len + __builtin_ctz(bmask);
   }

   static inline int wordlen(const char *word)
   {
      if (!*word)
         return 0;

      unsigned bmask;

      if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmplt_epi8(_mm_abs_epi8(_mm_lddqu_si128((__m128i *)word)), obl16)))
         return __builtin_ctz(bmask);

      for (int len = 16 - (intptr_t)word%16;; len += 16)
         if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmplt_epi8(_mm_abs_epi8(_mm_load_si128((__m128i *)&word[len])), obl16)))
            return len + __builtin_ctz(bmask);
   }

   static inline int blanklen(const char *blank)
   {
      if (!*blank)
         return 0;

      unsigned bmask;

      if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpgt_epi8(_mm_abs_epi8(_mm_lddqu_si128((__m128i *)blank)), obl16)))
         return __builtin_ctz(bmask);

      for (int len = 16 - (intptr_t)blank%16;; len += 16)
         if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpgt_epi8(_mm_abs_epi8(_mm_load_si128((__m128i *)&blank[len])), obl16)))
            return len + __builtin_ctz(bmask);
   }


   // String copying from src to dst.
   // m: Max. capacity of dst, including the final nul.
   //    A value of 0 would indicate that the capacity of dst matches the size of src (including nul)
   // l: On entry, src length or 0, on exit, the length of src, maybe NULL
   // Returns the length of the resulting string in dst.
   static inline int strmlcpy(char *dst, const char *src, int m, int *l)
   {
      int k, n;

      if (l)
      {
         if (!*l)
            *l = strvlen(src);
         k = *l;
      }
      else
         k = strvlen(src);

      if (!m)
         n = k;
      else
         n = (k < m) ? k : m-1;

      switch (n)
      {
         default:
            if ((intptr_t)dst&0xF || (intptr_t)src&0xF)
               for (k = 0; k  < n>>4<<1; k += 2)
                  ((uint64_t *)dst)[k] = ((uint64_t *)src)[k], ((uint64_t *)dst)[k+1] = ((uint64_t *)src)[k+1];
            else
               for (k = 0; k  < n>>4; k++)
                  _mm_store_si128(&((__m128i *)dst)[k], _mm_load_si128(&((__m128i *)src)[k]));
         case 8 ... 15:
            if ((k = n>>4<<1) < n>>3)
               ((uint64_t *)dst)[k] = ((uint64_t *)src)[k];
         case 4 ... 7:
            if ((k = n>>3<<1) < n>>2)
               ((uint32_t *)dst)[k] = ((uint32_t *)src)[k];
         case 2 ... 3:
            if ((k = n>>2<<1) < n>>1)
               ((uint16_t *)dst)[k] = ((uint16_t *)src)[k];
         case 1:
            if ((k = n>>1<<1) < n)
               dst[k] = src[k];
         case 0:
            ;
      }

      dst[n] = '\0';
      return n;
   }

#else

   #define strvlen(s) strlen(s)

   static inline int linelen(const char *line)
   {
      if (!*line)
         return 0;

      int l;
      for (l = 0; line[l] && line[l] != '\n'; l++)
         ;
      return l;
   }

   static inline int taglen(const char *tag)
   {
      if (!*tag)
         return 0;

      int l;
      for (l = 0; tag[l] && tag[l] != ':'; l++)
         ;
      return l;
   }

   static inline int fieldlen(const char *field)
   {
      if (!*field)
         return 0;

      int l;
      for (l = 0; field[l] && field[l] != '|'; l++)
         ;
      return l;
   }

   static inline int wordlen(const char *word)
   {
      if (!*word)
         return 0;

      int l;
      for (l = 0; word[l] > ' '; l++)
         ;
      return l;
   }

   static inline int blanklen(const char *blank)
   {
      if (!*blank)
         return 0;

      int l;
      for (l = 0; blank[l] > ' '; l++)
         ;
      return l;
   }


   // String copying from src to dst.
   // m: Max. capacity of dst, including the final nul.
   //    A value of 0 would indicate that the capacity of dst matches the size of src (including nul)
   // l: On entry, src length or 0, on exit, the length of src, maybe NULL
   // Returns the length of the resulting string in dst.
   static inline int strmlcpy(char *dst, const char *src, int m, int *l)
   {
      int k, n;

      if (l)
      {
         if (!*l)
            *l = (int)strvlen(src);
         k = *l;
      }
      else
         k = (int)strvlen(src);

      if (!m)
         n = k;
      else
         n = (k < m) ? k : m-1;

      strlcpy(dst, src, m);
      return n;
   }

#endif

// forward skip white space  !!! s MUST NOT be NULL !!!
static inline char *skip(char *s)
{
   for (;;)
      switch (*s)
      {
         case '\t':
         case '\n':
         case '\r':
         case ' ':
            s++;
            break;

         default:
            return s;
      }
}

// backward skip white space  !!! s MUST NOT be NULL !!!
static inline char *bskip(char *s)
{
   for (;;)
      switch (*--s)
      {
         case '\t':
         case '\n':
         case '\r':
         case ' ':
            break;

         default:
            return s+1;
      }
}

static inline char *trim(char *s)
{
   *bskip(s+strvlen(s)) = '\0';
   return skip(s);
}


static inline char *lowercase(char *s, int n)
{
   if (s)
   {
      char c, *p = s;
      for (int i = 0; i < n && (c = *p); i++)
         if ('A' <= c && c <= 'Z')
            *p++ = c + 0x20;
         else
            p++;
   }
   return s;
}

static inline char *uppercase(char *s, int n)
{
   if (s)
   {
      char c, *p = s;
      for (int i = 0; i < n && (c = *p); i++)
         if ('a' <= c && c <= 'z')
            *p++ = c - 0x20;
         else
            p++;
   }
   return s;
}

typedef long long          llong;

typedef unsigned char      uchar;
typedef unsigned short     ushort;
typedef unsigned int       uint;
typedef unsigned long      ulong;
typedef unsigned long long ullong;

typedef unsigned int       utf8;
typedef unsigned int       utf32;


// void pointer reference
#define VPR(p) (void **)&(p)

typedef struct
{
   ssize_t size;
   size_t  check;
   char    payload[16];
// size_t  zerowall;       // the allocation routines allocate sizeof(size_t) extra space and set this to zero
} allocation;

#define allocationMetaSize (offsetof(allocation, payload) - offsetof(allocation, size))

extern ssize_t gAllocationTotal;

void *allocate(ssize_t size, bool cleanout);
void *reallocate(void *p, ssize_t size, bool cleanout, bool free_on_error);
void deallocate(void **p, bool cleanout);
void deallocate_batch(bool cleanout, ...);



#pragma mark ••• AVL Tree of IPv4-Ranges •••

typedef union
{
   uint8_t   nibble[4];
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

typedef __uint128_t uint128_t;

typedef union
{
   uint64_t  chunk[2];
   uint16_t  nibble[8];
   uint128_t number;
} IP6Desc;

typedef uint128_t IP6Set[3];  // [0] -> lo, [1] -> hi, [2] -> cc

typedef struct IP6Node
{
   uint128_t lo, hi;          // IPv4 number range
   uint32_t  cc;              // country code

   int32_t B;                 // house holding
   struct IP6Node *L, *R;
} IP6Node;

IP6Node  *findIP6Node(uint128_t ip, IP6Node  *node);
IP6Node *findNet6Node(uint128_t lo, uint128_t hi, uint32_t cc, IP6Node  *node);
int        addIP6Node(uint128_t lo, uint128_t hi, uint32_t cc, IP6Node **node);
int     removeIP6Node(uint128_t ip, IP6Node **node);
void serializeIP6Tree(FILE *out, IP6Node *node);
void   releaseIP6Tree(IP6Node *node);

static inline int bisectionIP6Search(uint128_t ip6, IP6Set *sortedIP6Sets, int count)
{
   uint128_t u;
   int o, p, q;
   for (p = 0, q = count-1; p <= q;)
   {
      o = (p + q) >> 1;
      if ((u = sortedIP6Sets[o][0]) <= ip6 && ip6 <= sortedIP6Sets[o][1])
         return o;
      else if (ip6 < u)
         q = o-1;
      else // (ip6 > sortedIP6Sets[o][1])
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

CCNode **createCCTable(void);
void    releaseCCTable(CCNode *table[]);

CCNode *findCC(CCNode *table[], uint32_t cc);
void   storeCC(CCNode *table[], char *ccui);
void  removeCC(CCNode *table[], uint32_t cc);



#pragma mark ••• More utility functions •••

static inline int maxi(int a, int b)
{
   return (a > b) ? a : b;
}

static inline int ccEncode(uint16_t cc)
{
   uint8_t *ca = (uint8_t *)&cc;
   return (ca[b2_0]-60)*1000 + ca[b2_1]*10;
}


#include <sys/socket.h>
#include <arpa/inet.h>

static inline uint32_t ipv4_str2bin(char *str)
{
   uint32_t bin;
   return (inet_pton(AF_INET, str, &bin) > 0)
          ? swapInt32(bin)
          : 0;
}

typedef char IP4Str[16];
static inline char *ipv4_bin2str(uint32_t bin, char *str)
{
   IP4Desc ipdsc = {.number = bin};
   sprintf(str, "%d.%d.%d.%d", ipdsc.nibble[b4_3], ipdsc.nibble[b4_2], ipdsc.nibble[b4_1], ipdsc.nibble[b4_0]);
   return str;
}

static inline uint128_t ipv6_str2bin(char *str)
{
   uint64_t bin[2];
   return (inet_pton(AF_INET6, str, &bin) > 0)
          ? (IP6Desc){swapInt64(bin[b2_1]), swapInt64(bin[b2_0])}.number
          : 0;
}

typedef char IP6Str[40];
static inline char *ipv6_bin2str(uint128_t bin, char *str)
{
   IP6Desc ipdsc = {.number = bin};
   sprintf(str, "%x:%x:%x:%x:%x:%x:%x:%x", ipdsc.nibble[b8_7], ipdsc.nibble[b8_6], ipdsc.nibble[b8_5], ipdsc.nibble[b8_4],
                                           ipdsc.nibble[b8_3], ipdsc.nibble[b8_2], ipdsc.nibble[b8_1], ipdsc.nibble[b8_0]);
   return str;
}


static const uint32_t v4[33] =
{
   0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
   0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF, 0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF, 0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF,
   0xFFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
};

static inline uint32_t inteb4(int32_t e)
{
   return (0 <= e && e <= 31) ? v4[e]+1 : 1;
}

static inline int32_t intlb4(double v)
{
   return (int32_t)log2(v);
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

static inline uint128_t inteb6(int32_t e)
{
   return (0 <= e && e <= 127) ? v6[e].number+1 : 1;
}

static inline int32_t intlb6(uint128_t v)
{
   uint128_t u;
   int o, p, q;
   for (p = 0, q = 127; p <= q;)
   {
      o = (p + q) >> 1;
      if ((u = v6[o].number) < v && v <= v6[o+1].number)
         return o;
      else if (v <= u)
         q = o-1;
      else // (v > v6[o+1])
         p = o+1;
   }

   return 0;
}
