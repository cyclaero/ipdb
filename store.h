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

   #define b32_0 0
   #define b32_1 1
   #define b32_2 2
   #define b32_3 3

   #define b64_0 0

#else

   #define b32_0 3
   #define b32_1 2
   #define b32_2 1
   #define b32_3 0

   #define b64_0 7

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


static inline char *skip(char *s)
{
   if (s) for (;;)
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

   return NULL;
}

static inline char *bskip(char *s)
{
   if (s) for (;;)
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

   return NULL;
}

static inline char *pass(char *s)
{
   bool sq = false, dq = false, qu = false;

   if (s) for (;;)
      switch (*s++)
      {
         case '\0':
            return s-1;

         case '\t':
         case '\n':
         case '\r':
         case ' ':
            if (!qu)
               return s-1;
            else
               break;

         case '\'':
            if (!dq)
               qu = sq = !sq;
            break;

         case '"':
            if (!sq)
               qu = dq = !dq;
            break;

         default:
            break;
      }

   return NULL;
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
} IPv4Desc;

typedef uint32_t IPSet[3]; // [0] -> lo, [1] -> hi, [2] -> cc

typedef struct IPNode
{
   uint32_t lo, hi;        // IP number range
   uint32_t cc;            // country code

   int32_t B;              // house holding
   struct IPNode *L, *R;
} IPNode;

IPNode   *findIPNode(uint32_t ip, IPNode  *node);
IPNode  *findNetNode(uint32_t lo, uint32_t hi, uint32_t cc, IPNode  *node);
int        addIPNode(uint32_t lo, uint32_t hi, uint32_t cc, IPNode **node);
void    importIPNode(uint32_t lo, uint32_t hi, uint32_t cc, IPNode **node);
int     removeIPNode(uint32_t ip, IPNode **node);

void serializeIPTree(FILE *out, IPNode *node);
void   releaseIPTree(IPNode *node);
int treeHeight(IPNode *node);
bool checkBalance(IPNode *node);
IPNode *sortedIPSetsToTree(IPSet *sortedIPSets, int start, int end);


#pragma mark ••• AVL Tree of Country Codes •••

typedef struct CCNode
{
   uint32_t cc;            // country code

   int32_t B;              // house holding
   struct CCNode *L, *R;
} CCNode;

CCNode *findCCNode(uint32_t cc, CCNode  *node);
int      addCCNode(uint32_t cc, CCNode **node);
int   removeCCNode(uint32_t cc, CCNode **node);
void releaseCCTree(CCNode *node);

#pragma mark ••• Pseudo Hash Table of Country Codes •••

CCNode **createCCTable(void);
void    releaseCCTable(CCNode *table[]);

CCNode *findCC(CCNode *table[], uint32_t cc);
void   storeCC(CCNode *table[], uint32_t cc);
void  removeCC(CCNode *table[], uint32_t cc);
