//  uint128t.h
//  ipdbtools
//
//  Created by Dr. Rolf Jansen on 2018-09-29.
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


#pragma mark ••• uint128 Arithmetic •••

typedef struct
{
   uint64_t quad[2];
} uint128s;

#if ((defined(__x86_64__) || defined(__arm64__))) && !defined(UInt128_Testing)

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

   static inline uint128t inc_u128(uint128t *a)
   {
      return ++(*a);
   }

   static inline uint128t dec_u128(uint128t *a)
   {
      return --(*a);
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
            a.quad[b2_1] = a.quad[b2_0] << (n - 64),                         a.quad[b2_0] = 0;
         else if (n < 64)
            a.quad[b2_1] = (a.quad[b2_1] << n) | (a.quad[b2_0] >> (64 - n)), a.quad[b2_0] = a.quad[b2_0] << n;
         else // (n == 64)
            a.quad[b2_1] = a.quad[b2_0],                                     a.quad[b2_0] = 0;
      }
      return a;
   }

   static inline uint128t shr_u128(uint128t a, uint32_t n)
   {
      if (n &= 0x7F)
      {
         if (n > 64)
            a.quad[b2_0] = a.quad[b2_1] >> (n - 64),                         a.quad[b2_1] = 0;
         else if (n < 64)
            a.quad[b2_0] = (a.quad[b2_0] >> n) | (a.quad[b2_1] << (64 - n)), a.quad[b2_1] = a.quad[b2_1] >> n;
         else // (n == 64)
            a.quad[b2_0] = a.quad[b2_1],                                     a.quad[b2_1] = 0;
      }
      return a;
   }

   static inline uint128t inc_u128(uint128t *a)
   {
      if (++(a->quad[b2_0]) == 0)
         (a->quad[b2_1])++;
      return *a;
   }

   static inline uint128t dec_u128(uint128t *a)
   {
      if ((a->quad[b2_0])-- == 0)
         (a->quad[b2_1])--;
      return *a;
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
