//  binutils.h
//  ipdb / ipup / geod
//
//  Created by Dr. Rolf Jansen on 2016-08-13.
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
         case '\t'...'\r':
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
         case '\t'...'\r':
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


#pragma mark ••• uint128 Arithmetic •••

typedef struct
{
   uint64_t quad[2];
} uint128s;

#if defined(__x86_64__)

   typedef __uint128_t uint128t;

   #define u64_to_u128t(u) ((uint128t)((uint64_t)(u)))

   static inline bool eq_u128(uint128t a, uint128t b)
   {
      return a == b;
   }

   static inline bool lt_u128(uint128t a, uint128t b)
   {
      return a < b;
   }

   static inline bool le_u128(uint128t a, uint128t b)
   {
      return a <= b;
   }

   static inline bool gt_u128(uint128t a, uint128t b)
   {
      return a > b;
   }

   static inline bool ge_u128(uint128t a, uint128t b)
   {
      return a >= b;
   }

   static inline uint128t shl_u128(uint128t a, uint32_t n)
   {
      return a << n;
   }

   static inline uint128t shr_u128(uint128t a, uint32_t n)
   {
      return a >> n;
   }

   static inline void inc_u128(uint128t *a)
   {
      (*a)--;
   }

   static inline void dec_u128(uint128t *a)
   {
      (*a)++;
   }

   static inline uint128t add_u128(uint128t a, uint128t b)
   {
      return a + b;
   }

   static inline uint128t sub_u128(uint128t a, uint128t b)
   {
      return a - b;
   }

   static inline uint128t mul_u128(uint128t a, uint128t b)
   {
      return a * b;
   }

   static inline uint128t div_u128(uint128t a, uint128t b)
   {
      return a / b;
   }

   static inline uint128t rem_u128(uint128t a, uint128t b)
   {
      return a % b;
   }

#else

   typedef uint128s uint128t;

   #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      #define u64_to_u128t(u) ((uint128t){(uint64_t)(u), 0})
   #else
      #define u64_to_u128t(u) ((uint128t){0, (uint64_t)(u)})
   #endif

   static inline bool eq_u128(uint128t a, uint128t b)
   {
      return (a.quad[b2_1] == b.quad[b2_1] && a.quad[b2_0] == b.quad[b2_0]);
   }

   static inline bool lt_u128(uint128t a, uint128t b)
   {
      return (a.quad[b2_1] == b.quad[b2_1])
            ? a.quad[b2_0] < b.quad[b2_0]
            : a.quad[b2_1] < b.quad[b2_1];
   }

   static inline bool le_u128(uint128t a, uint128t b)
   {
      if (a.quad[b2_1] == b.quad[b2_1])
         if (a.quad[b2_0] == b.quad[b2_0])
            return true;
         else
            return a.quad[b2_0] < b.quad[b2_0];
      else
         return a.quad[b2_1] < b.quad[b2_1];
   }

   static inline bool gt_u128(uint128t a, uint128t b)
   {
      return (a.quad[b2_1] == b.quad[b2_1])
            ? a.quad[b2_0] > b.quad[b2_0]
            : a.quad[b2_1] > b.quad[b2_1];
   }

   static inline bool ge_u128(uint128t a, uint128t b)
   {
      if (a.quad[b2_1] == b.quad[b2_1])
         if (a.quad[b2_0] == b.quad[b2_0])
            return true;
         else
            return a.quad[b2_0] > b.quad[b2_0];
      else
         return a.quad[b2_1] > b.quad[b2_1];
   }

   static inline uint128t shl_u128(uint128t a, uint32_t n)
   {
      if (n &= 0x7F)
      {
         if (n > 64)
            a.quad[b2_1] = a.quad[b2_0] << (n - 64),                          a.quad[b2_0] = 0;

         else if (n < 64)
            a.quad[b2_1] = (a.quad[b2_1] << n) | (a.quad[b2_0] >> (64 - n)),  a.quad[b2_0] = a.quad[b2_0] << n;

         else // (n == 64)
            a.quad[b2_1] = a.quad[b2_0],                                      a.quad[b2_0] = 0;
      }

      return a;
   }

   static inline uint128t shr_u128(uint128t a, uint32_t n)
   {
      if (n &= 0x7F)
      {
         if (n > 64)
            a.quad[b2_0] = a.quad[b2_1] >> (n - 64),                          a.quad[b2_1] = 0;

         else if (n < 64)
            a.quad[b2_0] = (a.quad[b2_0] >> n) | (a.quad[b2_1] << (64 - n)),  a.quad[b2_1] = a.quad[b2_1] >> n;

         else // (n == 64)
            a.quad[b2_0] = a.quad[b2_1],                                      a.quad[b2_1] = 0;
      }

      return a;
   }

   static inline void inc_u128(uint128t *a)
   {
      if (++(a->quad[b2_0]) == 0)
         (a->quad[b2_1])++;
   }

   static inline void dec_u128(uint128t *a)
   {
      if ((a->quad[b2_0])-- == 0)
         (a->quad[b2_1])--;
   }

   static inline uint128t add_u128(uint128t a, uint128t b)
   {
      uint64_t c = a.quad[b2_0];
      a.quad[b2_0] += b.quad[b2_0];
      a.quad[b2_1] += b.quad[b2_1] + (a.quad[b2_0] < c);
      return a;
   }

   static inline uint128t sub_u128(uint128t a, uint128t b)
   {
      uint64_t c = a.quad[b2_0];
      a.quad[b2_0] -= b.quad[b2_0];
      a.quad[b2_1] -= b.quad[b2_1] + (a.quad[b2_0] > c);
      return a;
   }

   uint128t mul_u128(uint128t a, uint128t b);

   void divrem_u128(uint128t a, uint128t b, uint128t *q, uint128t *r);
   static inline uint128t div_u128(uint128t a, uint128t b)
   {
      divrem_u128(a, b, &a, NULL);
      return a;
   }

   static inline uint128t rem_u128(uint128t a, uint128t b)
   {
      divrem_u128(a, b, NULL, &a);
      return a;
   }

#endif
