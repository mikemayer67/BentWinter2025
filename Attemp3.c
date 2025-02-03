//
//  main.c
//  CompBonusC
//
//  Created by Mike Mayer on 1/14/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

//-------------------------------------------------------------------------------------------------
// Problem Statement
//-------------------------------------------------------------------------------------------------
//
//   A number in a particular base is palindromic if the digits (no leading zeros allowed)
//   read the same right to left as left to right. For each integer BASE N > 2, let P(N)
//   be the decimal representation of the smallest integer exceeding 2N that is palindromic
//   both in base N and in base 2.
//
//   Consider the sequence of numbers formed by including P(N) (as N is incremented by 1)
//   provided it is larger than any existing P(N) already in the sequence. To wit, P(3),
//   which can be represented as 100010001 in base 3 and 1100111110011 in base 2 (and
//   6643 as a decimal) is established as the first number. P(4) which can be represented
//   as 33 in base 4 and 1111 //   in base 2 (and 15 as a decimal), is smaller than P(3)
//   and, therefore, excluded from the sequence.
//
//   What is the first decimal numer in this->sequence larger than one quadrillion?
//   What is the corresponding N and which number in the sequence is it?
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Design Notes
//-------------------------------------------------------------------------------------------------
// - History
//   - This is the third attempt to solve this problem
//   - python implementation, took over a minute to get P(N) > 1 million
//   - C++ implentation, got to 1 billion in just over 2 minutes
//
// - What's different here?
//   - pure C implementation
//   - all memory management occur during startup, no allocation/deallocation on the heap after that
//   - use of global variables to reduce stack initialization on function calls
//
// - Notation
//   - Digit_t  = a single digit within an arbitrary base number
//   - Bit_t    = a single digit (bit) within a binary number
//   - Size_t   = integer type suitable for array indexing (for this problem)
//   - Number_t = a struct containing a length (number of digits) and an array of digits
//   - Binary_t = a struct containing a length (number of bits) and an array of bits
//-------------------------------------------------------------------------------------------------

// Number of decimal digits to store 1 quadrillion
//   (number of zeros: thousand=3, million=6, billion=9, trillion=12, quadrillion=15)
#define DECLEN 16

// Number of binary bits to store 1 quadrillion
//   log2(1e15) = 49.82 -->  2^50 > 1e15  --> 51 digits
#define BINLEN 51

typedef int32_t Size_t;   // signed for negative index values in for loops
typedef uint32_t Digit_t;
typedef uint8_t  Bit_t;
typedef uint8_t  bool;

const bool true = 1;
const bool false = 0;

// Although globals are considered bad form, the need for runtime
//    efficiency is huge here... so we're going to use them to avoid
//    passing around pointers to functions (probably overkill, but...

// current base
Digit_t gN = 0;

// registers
//   CurNumber   - current Base N palindrome
//   CurBinary   - CurNumber converted to binary
//   Div2BufferA - first of two Base N registers for dividing by 2
//   Div2BufferB - first of two Base N registers for dividing by 2

typedef struct {
  Size_t  size;
  Digit_t digits[1+DECLEN];   // little-endian, extra digit just in case
} Number_t;

typedef struct {
  Size_t size;
  Bit_t  bits[5+BINLEN]; // extra bits just in case
} Binary_t;

Number_t gCurNumber;
Binary_t gCurBinary;
Number_t gDiv2BufferA;
Number_t gDiv2BufferB;

//

// result cache
// MaxBinary - current max N, expressed in binary
Binary_t gMaxBinary;

Bit_t divmod2(Number_t *d, Number_t *q)
{
  // This function divides the input dividend (d) by 2.
  // It does not modify the dividend.
  // It stores the quotient in q.
  // It returns the remainder (0 or 1)
  
  // start with the number of digits in the divisor
  Size_t   nd = d->size;
  Digit_t msd = d->digits[nd-1];
  
  Bit_t R = 0;
  switch(msd) {
    case 0:
      printf("\nRuntime Error.. encountered leading digit of 0");
      exit(1);
      break;
    case 1:
      // quotent will have one less digit than the divisor
      //  the leading 1 will need to roll to next significant digit
      nd = nd - 1;
      R = 1;
      break;
  }
  
  // set size of the quotient and then begin division, carrying remainder as needed
  q->size = nd;
  for(Size_t i = nd - 1; i>=0; --i) {
    Digit_t t = R*gN + d->digits[i];
    Digit_t Q = t / 2;
    q->digits[i] = Q;
    R = t - 2*Q;
  }
  
  // return the final remainder
  return R;
}

void update_binary(void)
{
  // set pointers to the two div2 buffers
  Number_t *a = &gDiv2BufferA;
  Number_t *b = &gDiv2BufferB;
  Number_t *t = NULL; // for swap
  
  Size_t nbits=0;
  Bit_t bit = divmod2(&gCurNumber,a);
  gCurBinary.bits[nbits++] = bit;
  while(a->size) {
    bit = divmod2(a,b);
    gCurBinary.bits[nbits++] = bit;
    t = a; a = b; b = t;  // swap and b
  }
  gCurBinary.size = nbits;
}

bool binary_is_palindrome(void)
{
  Size_t a = 0;
  Size_t b = gCurBinary.size - 1;
  bool rval = true;
  while( rval && a < b ) {
    rval = ( gCurBinary.bits[a++] == gCurBinary.bits[b--] );
  }
  return rval;
}

void next_palindrome(void)
{
  Size_t nd = gCurNumber.size;
  // nd:(a,b)  ->   3:(1,1), 4:(1,2), 5:(2,2), 6:(2,3)
  Size_t a = (nd-1)/2;
  Size_t b = (nd-1)-a;
  for( ; a >= 0; --a, ++b )
  {
    Digit_t d = 1 + gCurNumber.digits[a];
    if( d == gN ) {
      gCurNumber.digits[a] = 0;
      gCurNumber.digits[b] = 0;
    } else {
      gCurNumber.digits[a] = d;
      gCurNumber.digits[b] = d;
      break;
    }
  }
  if( gCurNumber.digits[0] == 0 ) {
    // need to increase length of the palindrome
    gCurNumber.size = nd+1;
    gCurNumber.digits[0] = 1;
    gCurNumber.digits[nd-1] = 0;
    gCurNumber.digits[nd] = 1;
  }
}

bool is_next_in_sequence(void) {
  // returns whether or not current binary is greater than maximum binary so far
  Size_t curSize = gCurBinary.size;
  Size_t maxSize = gMaxBinary.size;
  if(curSize > maxSize) { return true; }
  if(curSize < maxSize) { return false; }
  // We're going to take advantage of the fact that we know we are working with
  //   palindromes.  We'll treat this as big-endian even though it was generated
  //   in little-endian order.
  // We also know that the neither number starts with a 0, so we'll start search
  //   at second bit
  // Strictly speaking, we only need to check the first half of the bits... but
  //   checking all digits will only happen if the numbers are equal, and I
  //   suspect this will be rare enough to ignore.
  Bit_t *cur = gCurBinary.bits;
  Bit_t *end = cur + curSize;
  Bit_t *max = gMaxBinary.bits;
  for(; cur<end; ++cur, ++max) {
    if(*cur > *max) { return true; }
    if(*cur < *max) { return false; }
  }
  return false;  // aka equal
}

void update_max_binary(void) {
  gMaxBinary.size = gCurBinary.size;
  memcpy(gMaxBinary.bits, gCurBinary.bits, gCurBinary.size * sizeof(Bit_t));
}

void display_pn(Size_t seq) {
  printf("%d: %d\n",seq,gN);
}

int main(int argc, const char * argv[]) {
  // allocate the buffer memory
  
  gCurNumber.size=0;
  gDiv2BufferA.size=0;
  gDiv2BufferB.size=0;
  gCurBinary.size=0;
  gMaxBinary.size=0;
  
  // The following is a pseudo-infinite loop. gN will continue increasing
  //   until it exceeds the max value for a Digit_t and will then "reset"
  //   to 0.  At that point, the loop will terminate.  Hopefully, a solution
  //   will be found befor that happens and we will break out of the loop.
  Size_t seq = 0;
  for(gN=3; gN>1; ++gN)
  {
    // initialize the current number to 22 as this is the smallest
    //   palindrome (regardless of base) that exceeds 2N
    gCurNumber.size = 2;
    gCurNumber.digits[0] = 2;
    gCurNumber.digits[1] = 2;
    
    update_binary();
    while(!binary_is_palindrome())
    {
      next_palindrome();
      update_binary();
    }
    if(is_next_in_sequence())
    {
      update_max_binary();
      display_pn(++seq);
      if(seq>100) break;
    }
  }
  return 0;
}
