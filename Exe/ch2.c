#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
// 2.27
// Write a function to determine whether arguments can be added without oveflow
int uadd_ok(unsigned x, unsigned y) {
  return (x + y) < x ? 0: 1;
}

// 2.30
// Determine whether arguments can be added without overflow
int tadd_ok(int x, int y) {
  int sum = x + y;
  if (x > 0 && y > 0 && sum < 0) return false;
  if (x < 0 && y < 0 && sum > 0) return false;
  return true;
}

// 2.74
// Determine whether substracing arguments will cause oveflow
int tsub_ok(int x, int y) {
  if (y == -(1 << 31)) return 0;
  return tadd_ok(x, -y);
}

// 2.35
// Determine whether arguments can be multiplied without overflow
int tmult_ok(int x, int y) {
  int p = x * y;
  return !x || p / x == y;
}

// 2.36
int tmult_ok2(int x, int y) {
  long long p = (long long)x * y;
  return p == (int)p;
}

// 2.42
// Write a function div16 that returns the value x/16 for integer argument x.
// Your function should not use division, modulus, multiplication, any conditionals,
// any comparision operators, or any loops. You may assume that data type int is
// 32 bits long and uses a two's-complement representaiton, and that right shitfs
// are performed arithmetically.
int div16(int x) {
  // if x >= 0 bias = 0 else bias = 15
  int bias = (x >> 31) & 0xF;
  return (x + bias) >> 4;
}

// 2.83
unsigned f2u(float x) {
  return *((unsigned*)&x);
}

int float_ge(float x, float y) {
  unsigned ux = f2u(x);
  unsigned uy = f2u(y);
  unsigned sx = ux >> 31;
  unsigned sy = uy >> 31;
  return sx == sy ? (sx == 0 ? ux >= uy : ux <= uy) : sx < sy;
}

int main(int argc, char const *argv[])
{
  return 0;
}