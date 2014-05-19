/*
 * CS:APP Data Lab
 *
 * <Please put your name and userid here>
 *
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:

  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code
  must conform to the following style:

  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>

  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.


  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function.
     The max operator count is checked by dlc. Note that '=' is not
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 *
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce
 *      the correct answers.
 */


#endif
/*
 * bitAnd - x&y using only ~ and |
 *   Example: bitAnd(6, 5) = 4
 *   Legal ops: ~ |
 *   Max ops: 8
 *   Rating: 1
 */
int bitAnd(int x, int y) {
  /*
      Since A & B = ~(~(A & B)) and ~(A & B) = (~A) | (~B)
      thne A & B = ~((~A) | (~B))
   */
  return ~((~x) | (~y));
}
/*
 * getByte - Extract byte n from word x
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: getByte(0x12345678,1) = 0x56
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int getByte(int x, int n) {
  /*
      int bits = 8 * n;
      return (x >> bits) & 0xFF;

      we cannot use multiply so use shift instead
      => bits = 8 * n = n * 8 = n << 3
   */
  return (x >> (n << 3)) & 0xFF;
}

/*
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int logicalShift(int x, int n) {
  /*
    x >>= n;
    int mask = ~(~(~0 << n) << (32 - n));
    return x & mask;

    since we cannot use -, we use + ((~n) + 1) instead
  */
  x >>= n;
  return x & ~(~(~0 << n) << (32 + ~n + 1));
}
/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int bitCount(int x) {
  return 2;
}
/*
 * bang - Compute !x without using !
 *   Examples: bang(3) = 0, bang(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4
 */
int bang(int x) {
  /*
    the most significant bit of x and x - 1 will not be different
    except x = 0 or x = (1<<31) (due to overflow)
    to check whether the most significant bit is change we can
    (x ^ ((x - 1) >> 31)) & 0x1, if = 1, means change, otherwise not
    So if it is changed we can know x = 0 or (1 << 31)
    How can we get rid of the case (1 << 31): we have to make some change to
    the negative number.
    Since we know that -1 >> 1 = -1, so apply >> 1 to the negative number
   */

  // Only apply >> 1 to the negative number, if it is positive number keep
  // the original number
  x >>= (x >> 31) & 0x01;
  // x minus 1
  int x_m_1 = x + (~1) + 1; // x - 1
  return (x ^ x_m_1) >> 31 & 0x01;
}
/*
 * tmin - return minimum two's complement integer
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  return 1 << 31;
}
/*
 * fitsBits - return 1 if x can be represented as an
 *  n-bit, two's complement integer.
 *   1 <= n <= 32
 *   Examples: fitsBits(5,3) = 0, fitsBits(-4,3) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int fitsBits(int x, int n) {
  /*
    check -2^(n-1) <= x <= 2^(n-1) - 1
    ==> x + 2^(n-1) >= 0 &&  2^(n-1) - 1 - x >= 0
   */
  int m_1 = (~1) + 1; // -1
  int n_1 = n + m_1; // n - 1
  int n_min = (1 << n_1); // 1 << (n-1)
  int n_max = n_min + m_1; // 1 << (n-1) - 1
  int m_x = ~x + 1; // -x
  int mask = (1 << 31);
  // check sign
  return !((n_max + m_x) & mask) & ! ((x + n_min) & mask);
}
/*
 * divpwr2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: divpwr2(15,1) = 7, divpwr2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int divpwr2(int x, int n) {
    /*
      return x < 0 ? ( (x + (1 << n) -1) : x ) >> k;
     */
    int a = (x >> 31) & 0x01;
    return (x + (a << n) + (~a) + 1) >> n;
}
/*
 * negate - return -x
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x + 1;
}
/*
 * isPositive - return 1 if x > 0, return 0 otherwise
 *   Example: isPositive(-1) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 3
 */
int isPositive(int x) {
  /*
    if x >= 0 => most significant bit = 0
    if x > 0 => !!x = 0x01
   */
  return !(x >> 31) & !!x;
}
/*
 * isLessOrEqual - if x <= y  then return 1, else return 0
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  /*
    case 1: x == y, x - y = 0  => x + (~y) + 1 = 0
    case 2: x and y have different sign => x < 0
    case 3: x and y have same sign => y - x > 0
   */
  int sign_x = (x >> 31) & 0x1;
  int sign_y = (y >> 31) & 0x1;
  int sign_y_x = ((y + (~x) + 1) >> 31) & 0x1;
  return !(x+(~y)+1) | ((sign_x ^ sign_y) & sign_x) | (!(sign_x ^ sign_y) & !sign_y_x);
}
/*
 * ilog2 - return floor(log base 2 of x), where x > 0
 *   Example: ilog2(16) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 90
 *   Rating: 4
 */
int ilog2(int x) {
  // TODO: reduce operation
  int ret = 0;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  x >>= 1;
  ret += !!x;
  return ret;
}
/*
 * float_neg - Return bit-level equivalent of expression -f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 10
 *   Rating: 2
 */
unsigned float_neg(unsigned uf) {
 int t = (uf >> 22) & 0xFF;
 if (!(t ^ 0xFF) && (uf << 9) >> 9) {
   return uf;
 } else {
  return (1 << 31) ^ uf;
 }
}

/*
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
  unsigned ret = 0;
  int sign_x, Exp, frac;
  int MSB_pos, LSB_pos;
  int LSB_mask, mask;
  int half, round_num;
  int temp;

  unsigned mask_31 = 1 << 31;
  unsigned logical_shitf_mask = mask_31 - 1;

  // special case x = 0
  if (!x) return x;

  // Get Sign
  ret = ret | (x & mask_31);
  if (ret) {
    x = -x;
  }

  temp = x;
  MSB_pos = 0;
  while (temp = (temp >> 1) & logical_shitf_mask) {
    MSB_pos += 1;
  }
  // MSB_pos -= 1;

  // When MSB in the position of i (0-index),
  // we will need i bits for fraction to maintain the accuracy
  // In the case of single precise floating-point, we will have
  // 23 bits (0 ~ 22) for fraction number. So the Maximun Position of MSB
  // is 23 (0-index) due to the implied leading 1 presentation:
  // 1.abcdefg... => .abcdefg..., we dont need to store the MSB
  if (MSB_pos > 23) {
    // fraction bits are not engough,
    // so we have to perform rounding: Round-To-Even

    // The postion of LBS (0-index)
    LSB_pos = MSB_pos - 23;
    LSB_mask = 1 << LSB_pos;
    mask = LSB_mask - 1;
    half = 1 << (LSB_pos - 1);

    round_num = x & mask;
    x = x & ~(mask);
    if ( round_num > half || (round_num == half && (x & LSB_mask)) ) {
      x = x + LSB_mask;
    }
  }

  // recompute the cnt and MSB_pos
  MSB_pos = 0;
  temp = x;
  while (temp = (temp >> 1) & logical_shitf_mask) {
    // temp = (temp >> 1) & logical_shitf_mask;
    // if (!temp) break;
    MSB_pos += 1;
  }
  Exp = MSB_pos + 127;
  // shitf the MSB out then shit the new MSB into position 22 (0-index)
  frac = x << (32 - MSB_pos) >> 9; // 9 = 31 - 22

  // the right shitf should be logical shitft (add 0)
  frac = frac & ((1 << 23) - 1);

  return ret | Exp << 23 | frac;
}
/*
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
  return 2;
}
