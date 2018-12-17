/*

A binary gap within a positive integer N is any maximal sequence of
consecutive zeros that is surrounded by ones at both ends in the
binary representation of N.

For example, number 9 has binary representation 1001 and contains a
binary gap of length 2. The number 529 has binary representation
1000010001 and contains two binary gaps: one of length 4 and one of
length 3. The number 20 has binary representation 10100 and contains
one binary gap of length 1. The number 15 has binary representation
1111 and has no binary gaps. The number 32 has binary representation
100000 and has no binary gaps.

Write a function:

    int solution(int N);

that, given a positive integer N, returns the length of its longest
binary gap. The function should return 0 if N doesn't contain a binary
gap.

For example, given N = 1041 the function should return 5, because N
has binary representation 10000010001 and so its longest binary gap is
of length 5. Given N = 32 the function should return 0, because N has
binary representation '100000' and thus no binary gaps.

Write an efficient algorithm for the following assumptions:

        N is an integer within the range [1..2,147,483,647].

 */

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>
#include <stdexcept>

namespace {

// We need a count trailing zero's function.  Here are some
// implementations from slowest (with GCC/Clang on x86_64) to fastest.

// See <https://en.wikipedia.org/wiki/Find_first_set#CTZ>

constexpr unsigned ctz_simple(unsigned x)
{
  auto constexpr bits = sizeof(x) * CHAR_BIT;
  for (auto bit = 0u; bit < bits; ++bit) {
    if (x & 1)
      return bit;
    x >>= 1;
  }
  return bits;
}

// Lifted from:
// <https://github.com/p12tic/libbittwiddle>

template <class T>
constexpr T broadcast(std::uint8_t x)
{
  static_assert(CHAR_BIT == 8);
  // equivalent to 0x0101..01 * x
  return ((~T(0)) / 0xff) * x;
}

// See <http://aggregate.org/MAGIC/#Population%20Count%20(Ones%20Count)>

template <class T>
constexpr unsigned popcnt(T x)
{
  static_assert(!std::is_signed<T>::value);
  static_assert(sizeof(T) == 4);
  static_assert(CHAR_BIT == 8);

  T b_0x01 = broadcast<T>(0x01); // 0b00000001
  T b_0x55 = broadcast<T>(0x55); // 0b01010101
  T b_0x33 = broadcast<T>(0x33); // 0b00110011
  T b_0x0f = broadcast<T>(0x0f); // 0b00001111

  // sum adjacent bits
  x -= (x >> 1) & b_0x55;
  // sum adjacent pairs of bits
  x = (x & b_0x33) + ((x >> 2) & b_0x33);
  // sum adjacent quartets of bits
  x = (x + (x >> 4)) & b_0x0f;

  // sum all octets of bits
  return (x * b_0x01) >> (sizeof(T) - 1) * CHAR_BIT;
}

// See <http://aggregate.org/MAGIC/#Trailing%20Zero%20Count>

constexpr unsigned ctz_bits(unsigned x)
{
  // if we had a builtin popcnt, we'd have a builtin ctz
  return popcnt((x & -x) - 1);
}

constexpr unsigned ctz_bsearch(unsigned x)
{
  static_assert(sizeof(x) == 4);

  if (x == 0)
    return 32;

  auto n = 0;
  if ((x & 0x0000FFFF) == 0) {
    n += 16;
    x >>= 16;
  }
  if ((x & 0x000000FF) == 0) {
    n += 8;
    x >>= 8;
  }
  if ((x & 0x0000000F) == 0) {
    n += 4;
    x >>= 4;
  }
  if ((x & 0x00000003) == 0) {
    n += 2;
    x >>= 2;
  }
  if ((x & 0x00000001) == 0) {
    ++n;
  }
  return n;
}

constexpr int ctz_debruijn(unsigned x)
{
  static_assert(sizeof(x) == 4);

  if (x == 0)
    return 32;

  // <http://supertech.csail.mit.edu/papers/debruijn.pdf>

  constexpr uint32_t debruijn32{0x077CB531};

  // for (auto i = 0; i < 32; ++i) {
  //   index32[(debruijn32 * (1 << i)) >> 27 & 0x1F] = i;
  // }

  constexpr char index32[32]{
      0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
      31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9,
  };

  return index32[((x & (-x)) * debruijn32) >> 27 & 0x1F];
}

// Now this is the obvious answer: have the processor just do it.

#if defined(__GNUC__)

inline auto ctz(unsigned x) { return __builtin_ctz(x); }

#elif defined(_MSC_VER)

#warning The MSC code is untested

#include <intrin.h>

inline auto ctz(unsigned x)
{
  static_assert(sizeof(x) == 4);

  if (x == 0)
    return 32;

  int r = 0;
  _BitScanReverse(&r, x);
  return r;
}

#elif __has_include(<strings.h>)

#define _XOPEN_SOURCE 700
#include <strings.h>

inline auto ctz(unsigned x)
{
  static_assert(sizeof(x) == 4);

  if (x == 0)
    return 32;

  auto fs = ffs(x);
  return fs - 1;
}

#else

auto constexpr ctz = ctz_debruijn;
//                 = ctz_bsearch;
//                 = ctz_bits;
//                 = ctz_simple;

#endif

inline auto cto(unsigned x) // count trailing one bits
{
  return ctz(~x);
}

int solution(int N)
{
  if (N < 1) {
    throw std::out_of_range("N not a positive integer");
  }

  auto n = unsigned(N);

  n >>= ctz(n); // shift out trailing zeros
  n >>= cto(n); // shift out trailing ones

  int max = 0;

  while (n) {
    int const tz = ctz(n);   // next block of trailing zeros are gap bits
    max = std::max(tz, max); // keep track of the largest block of them
    n >>= tz;                // shift those zero bits out
    n >>= cto(n);            // shift out the next block of one bits
  }

  return max;
}
} // namespace

int main()
{
  auto constexpr REPS = 0xFF'FF'FF;

  auto total_bits = 0;
  for (auto i = 1; i < REPS; ++i) {
    total_bits += solution(i * 0x10 + i);
  }
  assert(total_bits == 68022587);

  assert(ctz(0) == 32);
  assert(ctz(0x80000000) == 31);
  assert(ctz(0x00000F00) == 8);
  assert(ctz(1) == 0);
  assert(ctz(0xF) == 0);
  assert(ctz(0xFF) == 0);
  assert(ctz(0xFFFFFFFF) == 0);

  auto constexpr count_gap_zeros = solution;

  // Specific return values from the problem description.
  assert(count_gap_zeros(9) == 2);
  assert(count_gap_zeros(529) == 4);
  assert(count_gap_zeros(15) == 0);
  assert(count_gap_zeros(32) == 0);
  assert(count_gap_zeros(1041) == 5);

  assert(count_gap_zeros(2'147'483'647) == 0);

  assert(count_gap_zeros(0b101) == 1);
  assert(count_gap_zeros(0b1001) == 2);
  assert(count_gap_zeros(0b10001) == 3);
  assert(count_gap_zeros(0b100001) == 4);
  assert(count_gap_zeros(0b1000001) == 5);
  assert(count_gap_zeros(0b10000001) == 6);
  assert(count_gap_zeros(0b100000001) == 7);
  assert(count_gap_zeros(0b1000000001) == 8);
  assert(count_gap_zeros(0b10000000001) == 9);
  assert(count_gap_zeros(0b100000000001) == 10);

  assert(count_gap_zeros(0b1010) == 1);
  assert(count_gap_zeros(0b10010) == 2);
  assert(count_gap_zeros(0b100010) == 3);
  assert(count_gap_zeros(0b1000010) == 4);
  assert(count_gap_zeros(0b10000010) == 5);
  assert(count_gap_zeros(0b100000010) == 6);
  assert(count_gap_zeros(0b1000000010) == 7);
  assert(count_gap_zeros(0b10000000010) == 8);
  assert(count_gap_zeros(0b100000000010) == 9);
  assert(count_gap_zeros(0b1000000000010) == 10);

  assert(count_gap_zeros(0x55'55'55'55) == 1);
  assert(count_gap_zeros(0x2A'AA'AA'AA) == 1);

  assert(count_gap_zeros(0x9'99'99'99) == 2);
  assert(count_gap_zeros(0x66) == 2);
  assert(count_gap_zeros(0x66'66'66) == 2);

  assert(count_gap_zeros(0b1000000000011) == 10);
  assert(count_gap_zeros(0b1000000000101) == 9);
  assert(count_gap_zeros(0b1000000001001) == 8);
  assert(count_gap_zeros(0b1000000010001) == 7);
  assert(count_gap_zeros(0b1000000100001) == 6);
  assert(count_gap_zeros(0b1000001000001) == 5);
  assert(count_gap_zeros(0b1000010000001) == 6);
  assert(count_gap_zeros(0b1000100000001) == 7);
  assert(count_gap_zeros(0b1001000000001) == 8);
  assert(count_gap_zeros(0b1010000000001) == 9);
  assert(count_gap_zeros(0b1100000000001) == 10);

  // assert(count_gap_zeros(0xFFFFFFFF) == -1);
  assert(count_gap_zeros(0x7FFFFFFF) == 0);
  assert(count_gap_zeros(0x3FFFFFFF) == 0);
  assert(count_gap_zeros(0x1FFFFFFF) == 0);

  assert(count_gap_zeros(0x7FFFFFFD) == 1);
  assert(count_gap_zeros(0x7FFFFFFB) == 1);
  assert(count_gap_zeros(0x7FFFFFF7) == 1);
  assert(count_gap_zeros(0x7FFFFFEF) == 1);
  assert(count_gap_zeros(0x7FFFFFDF) == 1);
  assert(count_gap_zeros(0x7FFFFFBF) == 1);
  assert(count_gap_zeros(0x7FFFFF7F) == 1);

  assert(count_gap_zeros(0x7FFFFFF9) == 2);

  return 0;
}
