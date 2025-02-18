//
//  main.cpp
//  CompBonusBinary
//
//  Created by Mike Mayer on 1/30/25.
//

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
//   This is the fourth attempt to solve this problem
//   1. python implementation, took over a minute to get P(N) > 1 million
//   2. C++ implentation, got to 1 billion in just over 2 minutes
//   3. C implementation did only marginally better than the C++ implementation
//
// - What's different here?
//   - No longer creating arrays of base-N digits
//   - No longer working in any particular base (other than the internal binary storage)
//   - A 64-bit unsigned int can store any number from 0 to 2^64-1
//      - 2^64 = (2^10)^6 * 2^4 = 16 * (1024)^6, which is slightly larger than 1.6e19
//      - 1 quadrillion = 1e15
//      - unless P(N) completely jumps over the quadrillions into the 10s of quintillions, we should be ok
//   - We can generate Base-N palindromes directly through a strategic sequences of additions
//      - the algorithm for generating the palindromes is described at the end of this file
//   - We are going WAY BACK here and using a pseudo Turing machine to encode the sequence
//      - A new sequence machine is needed for each base examined
//-------------------------------------------------------------------------------------------------

#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <vector>
#include <string>

typedef uint64_t Number_t;
typedef uint64_t Count_t;

std::string hh_mm_ss(time_t t)
{
  // used for adding a timestep to each P(N) displayed to stdout
  // converts a time (in seconds) to (hours):(minutes):(seconds)
  std::stringstream rval;
  rval << std::setw(2) << std::setfill('0') << (t/3600)
    << ":" << std::setw(2) << std::setfill('0')  << ((t%3600)/60)
    << ":" << std::setw(2) << std::setfill('0')  << (t%60);
  return rval.str();
}

std::string add_commas(Number_t n)
{
  // used for displaying the decimal values of P(N) displayed to stdout
  // adds commas between each group of 3 digits (units, thousands, millions, etc.)
  std::vector<int> blocks;
  while(n > 1000) {
    blocks.push_back(n%1000);
    n /= 1000;
  }
  std::stringstream rval;
  rval << n;
  for(auto bi = blocks.rbegin(); bi!=blocks.rend(); ++bi) {
    rval << "," << std::setw(3) << std::setfill('0') << *bi;
  }
  return rval.str();
}

std::string base_n_str(Number_t n, Number_t N)
{
  // used for displaying P(N) to stdout in a given base N (which could be 2 for binary)
  // we take dvantage of the fact we know n is a base N (or 2) palindrome and actually 
  //   display the number in a little-endian order (which matches the traditional big-endian
  //   order for a palindrome).
  std::stringstream rval;
  while(n) {
    if(N<=10) { rval << n%N; }
    else      { rval << "(" << (n%N) << ")"; }
    n /= N;
  }
  return rval.str();
}

class Operation
{
  // Operation is the base class for all atomic and composite
  //   operations in a palindrome generation sequence
public:
  // the step function updates the provided palindrome and returns
  // whether or not there are more iteration required to complete the operation:
  //   true if the operation is NOT complete
  //   false if the operation IS complete
  virtual bool step(Number_t &palindrome) = 0;
  virtual ~Operation() {}
};

// we're going to include these in std containers, to make this more readable,
//   we'll define the following typedefs
typedef Operation            *OpPtr_t;
typedef std::vector<OpPtr_t>  OpList_t;
typedef OpList_t::iterator    OpIter_t;


class Increment : public Operation {
  // The Increment operation updates the palindrom by adding a
  //   specified increment (adder) a specified number of times (n).
  // Each call to the step method makes a single addition.
  // The operation will complete after being invoked n times.
  // the operation resets after completion, ready for another series of invocations.

private:
  const Number_t _adder;    // number to add each step
  const Count_t  _repeat;   // number of times to repeat the addition
  Count_t        _counter;  // number of times the addition has been done so far

public:
  // constructor simply set the initial values fo each attribute
  Increment(Count_t n,Number_t adder) : _adder(adder), _repeat(n), _counter(0)
  {}

  virtual bool step(Number_t &palindrome)
  {
    // let's make sure we don't overflow our 64 bit limit
    if(palindrome > ULLONG_MAX - _adder) {
      std::cout << "64bit is insufficient" << std::endl;
      exit(1);
    }
    // perform the addition and increment the counter
    palindrome += _adder;
    _counter += 1;

    // if we've performed the specified number of additions,
    //   reset for the next sequence and return false to indicate
    //   that we're done with the current operation.
    if (_counter == _repeat) {
      _counter = 0;
      return false; // no more to do
    }

    // return true to indicate that there are still more iterations
    //   required to complete the operation
    return true; // not yet done
  }
};


class PairedOps : public Operation {
  // The Paired class handles pairs of operations of the form n:[S,I],S
  //   S may be any operation type.
  //   I is the addition of a single integer value
  //   Note that the S op may require multiple invocations to complete.
  // The operation resets after completion, ready for another series of invocations.

private:
  bool     _on_S;  // flag indicating we are still working on completing S
  OpPtr_t  _S;     // pointer to the S operation
  Number_t _I;     // integer value to add during the I operation

  const Count_t  _repeat;   // number of times to repeat the [S,I] sub-sequence
  Count_t        _counter;  // number of times the [S,I] sub-sequence has been done so far

public:
  // constructor simply set the initial values fo each attribute
  PairedOps(Count_t n, OpPtr_t S, Number_t I)
  : _S(S), _I(I), _repeat(n), _counter(0), _on_S(true)
  {}

  virtual bool step(Number_t &palindrome)
  {
    // repeat the [S,I] sub-sequence n times
    if(_counter < _repeat) {
      if(_on_S) { // still in S
        // continue working on the S operation until it reports that it is complete
        if( ! _S->step(palindrome) ) {
          // S has been completed, clear _on_S flag
          _on_S = false;
        }
      } else {
        // S has been completed, add I
        // but first make sure we won't overflow 64 bits
        if(palindrome > ULLONG_MAX - _I) {
          std::cout << "64bit is insufficient" << std::endl;
          exit(1);
        }
        // add I, increment the [S,I] counter, and reset the _on_S flag
        palindrome += _I;
        _counter += 1;
        _on_S = true;
      }
    } else {
      // we've completed the n:[S,I] subsequence
      // we're working off the final S operation
      if( ! _S->step(palindrome) ) {
        // with completion of the final S operation, 
        //   we've completed the entire PairedOps operation
        // reset for the next use of this Operation
        //   and return false to indicate there is nothing more to do
        _counter = 0;
        _on_S = true;
        return false;
      }
    }

    // return true to indicate that there are still more iterations
    //   required to complete the operation
    return true;
  }
};


class Generator : public Operation {
  // Generates all of the palindromes with a specified base (N) and length (number of digits).
  // Its final step will generate the first palindrome of length+1.
  // The operation will complete after completing the generation sequence and the first 
  //   palindrom of the next length is returned.
  // The operation does not need to reset on completion as it will only be used once

private:
  bool _done;     // done with sequence, need to add 2 to increase number of digits
  OpList_t _S;    // list of all S ops needed for the generation operation
  OpPtr_t  _seq;  // the generation sequence for the current base and length

public:

  // the Generator constructor wraps all of the operations necessary to generate the palindromes
  //   of the specified length. (See algoithm at end of this file for details.)
  Generator(Number_t N, Count_t length) : _done(false)
  {
    // set the value of k based on the palindrom link
    //   length = 2k+1 (odd) or 2k+2 (even)
    Count_t k = (length+1)/2 - 1;
    bool odd = (length % 2 == 1);
    
    Number_t m = N-1;  // largest digit in Base-N
    Number_t q = m-1;  // second largest digit in Base-N
    
    if( length == 2 ) {
      // this one needs to be handled slightly differently as it starts with 22 rather than 11
      // sequence = (q-1):M11
      _seq = new Increment(q,N+1);
      _S.push_back(_seq);
    }
    else
    {
      // S0 = 1(odd) or 11(even) followed by k zeros
      Number_t M = odd ? N : N*(N+1);
      // I0 = 11 followed by k-1 zeros
      Number_t I = N+1;
      // add the k-1 zeros
      for(Count_t i=0; i<k-1; ++i) {
        M *= N;
        I *= N;
      }

      // build the sequence (see algorithm at the end of this file)
      OpPtr_t Si = new Increment(m,M);
      _S.push_back(Si);
      for(Count_t i=1; i<k; ++i) {
        Si = new PairedOps(m,Si,I);
        _S.push_back(Si);
        I /= N;
      }
      _seq = new PairedOps(q,Si,I);  // Sk
      _S.push_back(_seq);
    }
  }

  virtual bool step(Number_t &palindrome)
  {
    if(_done) {
      // done generating palindromes of current length (mmm...mmm)
      //   add 2 to go to first palindrom of next length (1000...001)
      // but only after verifying this won't overflow
      if(palindrome > ULLONG_MAX - 2) {
        std::cout << "64bit is insufficient" << std::endl;
        exit(1);
      }
      palindrome += 2;

      // reset (even though it's not necessary) and return false to indicate
      //   there are no more iterations in generation operation
      _done = false;
      return false; // complete
    } 
    else {
      // keep working through the generation sequence until it returns false to indicate completeion
      _done = ! _seq->step(palindrome);
    }
    // return true to indicate that we're not yet done
    return true;
  }

  ~Generator() 
  {
    // clean up the list of S operations
    for(auto op = _S.begin(); op!=_S.end(); ++op) { delete *op; }
  }
};

class IsBinaryPalindrome
{
  // Instances of this class are essentially callable functions
  //   that detetermine if a given number is a palindrome in base 2.
  // We take advantage of the fact that we know this will only be called
  //   with increasing numbers to keep track of the most significant bit (msb)
  //   so that we don't need to iterate through all bits looking for it on each call.
  //   We need only check to see if the msb needs to be increased (when it is less than
  //   the number being tested).

private:
  Number_t _msb;   // binary number with only the msb set to 1
  Number_t _mask;  // binary number with only the bits greater than msb set to 1

public:
  // constructor initializes the msb and mask to indicate a single bit binary number
  IsBinaryPalindrome(void) : _msb(1), _mask(~0x1) {}
  
  bool operator()(Number_t p) {
    // no even number can be a binary palindrome (leading 0's not allowed)
    if(p%2 == 0) { return false; }
    
    // The mask contains all the bits greater than the msb.  If p overlaps
    //  these bits, we have a new msb.  Shift both the msb and the mask by
    //  one until p no longer overlaps the mask.
    while(p & _mask) {
      _mask <<= 1;
      _msb <<= 1;
    }
    
    // Working from both the msb and lsb, look for differences in the
    //  bit values.  If any difference are found, not a palindrome
    //  If we get through all of the bits without finding a difference,
    //  we have a palindrome.
    Number_t a = _msb;
    Number_t b = 1;
    while(a>b) {
      Number_t t = a|b;  // palindrome test of the two bits of interest
      Number_t q = p&t;  // extract just those two bits
      if((q==0 || q==t)) { // check if both bits are 0 or both bits are 1
        // if so, shift the current test bits
        a >>= 1;
        b <<= 1;
      } else {
        // found a difference --> not a palindrome
        return false;
      }
    }
    // no differences found --> palindrome
    return true;
  }
};

Number_t calc_Pn(Number_t N)
{
  // Need a new IsBinaryPalindrome to reset the most significant bit
  //   An alternative would be a singleton with a reset method, but
  //   as this constructor is very light-weight, no need for that
  IsBinaryPalindrome is_binary_palindrome;
  
  // Iterate through increasing number of palindrome digits until
  //   a binary palindrome is found. Start with the two digit
  //   palindrome 22.  We don't actually need to check if this
  //   is a binary palindrome as it is an even number..
  Number_t p = N+1;
  for(Count_t L=2; true; ++L) { // yes, an infinte loop... we'll return from inside it
    Generator g(N,L);
    while( g.step(p) ) {
      if(is_binary_palindrome(p)) {
        return p;
      }
    }
  }
  return 0;  // we'll never get here, but to keep the compiler happy
}


int main(int argc, const char * argv[])
{
  time_t start_time = std::time(NULL);
  Number_t max_pn = 0;
  Number_t tgt_pn = 1000000000000000; // 1 quadrillion

  // Examine increaseing bases (N) util P(N) exceeds 1 quadrillion
  //   The upper bound in this for loop is purely to avoid an infitinite-loop.
  //   It is expected that the loop will be exited LONG before hitting this.
  for(Number_t N=3; N<tgt_pn; ++N)
  {
    Number_t pn = calc_Pn(N);
    if(pn > max_pn) {
      // only report P(N) values that are larger than the last P(N) value in the 
      //   solution sequence.
      std::cout 
        << hh_mm_ss(std::time(NULL) - start_time) << "  "
        << N << ": "
        << add_commas(pn) << ": "
        << base_n_str(pn,N) << " "
        << base_n_str(pn,2)
        << std::endl;
      
      max_pn = pn;

      if(pn >= tgt_pn) {
        // if P(N) exceeds 1 quadrillion, we're done... break out of the loop
        break;
      }
    }
  }
  std::cout << std:: endl;
  return 0;
}


//-------------------------------------------------------------------------------------------------
// # Mathematics and algorithm development for generating Base-N palindromes
//-------------------------------------------------------------------------------------------------
// In everything that follows, all numbers are in Base-N
// Let m = N-1, *the largest digit in base-N*
// Let q = m-1, *the second largest digit in base-N
// 
// Let's explore palindrome generation
// 
//  2 digit palindromes
//    - smallest is 22 (given problem statement that P(N) must exceed 2N)
//    - increment to next palindrome by adding 11
//    - continue until we hit mm (total of q-1 = m-2 = N-3 operations)
//       - 22 + 11 = 33
//       - 33 + 11 = 44
//       ...
//       - qq + 11 = mm
//   - sequence: [ (m-2):+11 ]
// 
//  3 digit palindromes
//    - move from 2 digit palindromes to 3 digits by adding 2
//      - mm + 2 = 101
//    - increment to next palindrome by adding 10
//    - continue until we hit 1m1 (total of m = N-1 operations)
//      - 101 + 10 = 111
//      - 111 + 10 = 121
//        ...
//      - 1q1 + 10 = 1m1
//    - at this point, we need to increment the 1st (and last) digit
//      - 1m1 + 11 = 202
//    - we need to repeat the above steps until we get to mmm
//      - 101 -> 202  [ m:10, 1:11 ]
//      - 202 -> 303  [ m:10, 1:11 ]
//        ...
//      - q0q -> m0m  [ m:10, 1:11 ]
//      - m0m -> mmm  [ m:10 ]       (note that we don't have the final addition of 11)
//    - sequence: [ 1:2, q:[m:10,1:11], m:10 ]
// 
//  4 digit palindromes
//    - move from 3 digit palindromes to 4 digits by adding 2
//      - mmm + 2 = 1001
//    - increment to next palindrome by adding 110
//    - continue until we hit 1mm1 (total of m = N-1 operations)
//      - 1001 + 110 = 1111
//      - 1111 + 110 = 1221
//        ...
//      - 1qq1 + 110 = 1mm1
//    - at this point, we need to increment the 1st (and last) digit
//      - 1mm1 + 11 = 2002
//    - we need to repeat the above steps until we get to mmmm
//      - 1001 -> 2002  [ m:110, 1:11 ]
//      - 2002 -> 3003  [ m:110, 1:11 ]
//        ...
//      - q00q -> m00m  [ m:110, 1:11 ]
//      - m00m -> mmmm  [ m:110 ]       (note that we don't have the final addition of 11)
//    - sequence: [ 1:2, q:[m:110,1:11], m:110 ]
// 
//  5 digit palindromes
//    -  mmmm -> 10001  [ 1:2 ]
//    --- step through 5 digit palindromes ---
//    - 10001 -> 10m01  [ m:100 ]
//    - 10m01 -> 11011  [ 1:110 ]
//    - 11011 -> 11m11  [ m:100 ]
//    - 11m11 -> 12021  [ 1:110 ]
//      ...
//    - 1q0q1 -> 1qmq1  [ m:100 ]
//    - 1qmq1 -> 1m0m1  [ 1:110 ]
//    - 1m0m1 -> 1mmm1  [ m:100 ]
//    --- collapse above and continue ---
//    - 10001 -> 1mmm1  [ m:[m:100,1:110], m:100 ]
//    - 1mmm1 -> 20002  [ 1:11 ]
//    - 20002 -> 2mmm2  [ m:[m:100,1:110], m:100 ]
//    - 2mmm2 -> 30003  [ 1:11 ]
//      ...
//    - m000m -> mmmmm  [ m:[m:100,1:110], m:100 ]
//    --- all together ---
//    sequence: [ 1:2, q:[ m:[m:100,1:110], m:100, 1:11 ], m:[m:100,1:110], m:100 ]
// 
//  6 digit palindromes
//    -  mmmmm -> 100001  [ 1:2 ]
//    --- step through 6 digit palindromes ---
//    - 100001 -> 10mm01  [ m:1100 ]
//    - 10mm01 -> 110011  [ 1:110 ]
//    - 110011 -> 11mm11  [ m:1100 ]
//    - 11mm11 -> 120021  [ 1:110 ]
//      ...
//    - 1q00q1 -> 1qmmq1  [ m:1100 ]
//    - 1qmmq1 -> 1m00m1  [ 1:110 ]
//    - 1m00m1 -> 1mmmm1  [ m:1100 ]
//    --- collapse above and continue ---
//    - 100001 -> 1mmmm1  [ m:[m:1100,1:110], m:1100 ]
//    - 1mmmm1 -> 200002  [ 1:11 ]
//    - 200002 -> 2mmmm2  [ m:[m:1100,1:110], m:1100 ]
//    - 2mmmm2 -> 300003  [ 1:11 ]
//      ...
//    - m0000m -> mmmmmm  [ m:[m:1100,1:110], m:1100 ]
//    --- all together ---
//    sequence: [ 1:2, q:[ m:[m:1100,1:110], m:1100, 1:11 ], m:[m:1100,1:110], m:1100 ]
// 
// ------
// At this point, note that each sequence for an even length palindrome is nearly identical
//   to the prior sequence for the odd length palindrome with the leading digit of the
//   adders doubled up (e.g. 100 -> 1100, 110 -> 1110, etc)
// Moving forward, we will only look at he odd length palindrom sequences
// ------
// 
//  7 digit palindromes
//    -  mmmmmm -> 1000001  [ 1:2 ]
//    --- step through 7 digit palindromes ---
//    - 1000001 -> 100m001  [ m:1000 ]
//    - 100m001 -> 1010101  [ 1:1100 ]
//    ...
//    - 10q0q01 -> 10qmq01  [ m:1000 ]
//    - 10qmq01 -> 10m0m01  [ 1:1100 ]
//    - 10m0m01 -> 10mmm01  [ m:1000 ]
//    --- collapse above and continue ---
//    - 1000001 -> 10mmm01  [ m:[m:1000, 1:1100], m:1000 ]
//    - 10mmm01 -> 1100011  [ 1:110 ]
//    - 1100011 -> 11mmm11  [ m:[m:1000, 1:1100], m:1000 ]
//    - 11mmm11 -> 1200021  [ 1:110 ]
//    ...
//    - 1m000m1 -> 1mmmmm1  [ m:[m:1000, 1:1100], m:1000 ]
//    --- collapse above and continue ---
//    - 1000001 -> 1mmmmm1  [ m:[ m:[m:1000, 1:1100], m:1000 ], 1:110, m:[m:1000, 1:1100], m:1000 ]
//    - 1mmmmm1 -> 2000002  [ 1:11 ]
//    ...
//    - m00000m -> mmmmmmm  [ m:[ m:[m:1000, 1:1100], m:1000 ], 1:110, m:[m:1000, 1:1100], m:1000 ]
//    --- all together ---
//    sequence: [ 1:2,
//                q:[ m:[ m:[m:1000, 1:1100], m:1000 ], 1:110, m:[m:1000, 1:1100], m:1000, 1:11] ],
//                    m:[ m:[m:1000, 1:1100], m:1000 ], 1:110, m:[m:1000, 1:1100], m:1000
//              ]
// ------
// ## Revised notation
//   This notation is become unsustainable...
//   Repeating above with a new level of sequence notation
// 
//   Define:
//     M#  = [m:#]
//     I#  = [1:#]
// ------
// 
//  7 digit palindromes
//    -  mmmmmm -> 1000001  I2
//    --- step through 7 digit palindromes ( S0 = M1000 )---
//    - 1000001 -> 100m001  S0
//    - 100m001 -> 1010101  I1100
//    ...
//    - 10q0q01 -> 10qmq01  S0
//    - 10qmq01 -> 10m0m01  I1100
//    - 10m0m01 -> 10mmm01  S0
//    --- collapse above and continue ( S1 = [ m:[S0,I1100], S0 ] )
//    - 1000001 -> 10mmm01  S1
//    - 10mmm01 -> 1100011  I110
//    - 1100011 -> 11mmm11  S1
//    - 11mmm11 -> 1200021  I110
//    ...
//    - 1m000m1 -> 1mmmmm1   S1
//    --- collapse above and continue ( S2 = [ m:[S1,I110], S1 ] )
//    - 1000001 -> 1mmmmm1  S2
//    - 1mmmmm1 -> 2000002  I11
//    - 2000002 -> 2mmmmm2  S2
//    - 2mmmmm2 -> 3000003  I11
//    ...
//    - m00000m -> mmmmmmm  S2
//    --- collapse above and continue ---
//    sequence: [ I2, q:[S2, I11], S2 ]
// 
//  8 digit palindromes (ok, I lied)
//    S0 = M11000
//    S1 = m:[S0,I1100],S0
//    S2 = m:[S1,I110],S1
//    sequence = I2, q:[S2,I11], S2
// 
//  9 digit palindromes
//    - mmmmmmmm -> 100000001 I2
//    --- step through 9 digit palindromes ( S0 = M10000 ) ---
//    - 100000001 -> 1000m0001 S0
//    - 1000m0001 -> 100101001 I11000
//    ...
//    - 100q0q001 -> 100qmq001 S0
//    - 100qmq001 -> 100m0m001 I11000
//    - 100m0m001 -> 100mmm001 S0
//    -- collapse above and continue (S1 = m:[S0,I11000],S0)
//    - 100000001 -> 100mmm001 S1
//    - 100mmm001 -> 101000101 I1100
//    ...
//    - 10q000q01 -> 10qmmmq01 S1
//    - 10qmmmq01 -> 10m000m01 I1100
//    - 10m000m01 -> 10mmmmm01 S1
//    -- collapse above and continue (S2 = m:[S1,I1100],S1)
//    - 100000001 -> 10mmmmm01 S2
//    - 10mmmmm01 -> 110000011 I110
//    ...
//    - 1q00000q1 -> 1qmmmmmq1 S2
//    - 1qmmmmmq1 -> 1m00000m1 I110
//    - 1m00000m1 -> 1mmmmmmm1 S2
//    -- collapse above and continue (S3 = m:[S2,I110],S2)
//    - 100000001 -> 1mmmmmmm1 S3
//    - 1mmmmmmm1 -> 200000002 I11
//    ...
//    - q0000000q -> qmmmmmmmq S3
//    - qmmmmmmmq -> m0000000m I11
//    - m0000000m -> mmmmmmmmm S3
//    --- collapse above and continue ---
//    sequence: I2, q:[S3,I1], S3
// 
//  10 digit palindromes
//    S0 = M110000
//    S1 = m:[S0,I11000],S0
//    S2 = m:[S1,I1100],S1
//    S3 = m:[S2,I110],S2
//    sequence = I2, q:[S3,I11],S3
// 
// ## Another leap
//  And now, I'm going to just extrapolate
// 
//  11 digit palindromes
//    S0 = M100000
//    S1 = m:[S0,I110000],S0
//    S2 = m:[S1,I11000],S1
//    S3 = m:[S2,I1100],S2
//    S4 = m:[S3,I110],S3
//    sequence = I2, q:[S4,I11], S4
// 
//  12 digit palindromes
//    S0 = M1100000
//    S1 = m:[S0,I110000],S0
//    S2 = m:[S1,I11000],S1
//    S3 = m:[S2,I1100],S2
//    S4 = m:[S3,I110],S3
//    sequence = I2, q:[S4,I11], S4
// 
// Let's go back and use the new notation for palindromes with less than 7 digits
//
//  6 digit palindromes
//    S0 = M1100
//    S1 = m:[S0,I110],S0
//    sequence = I2, q:[S1, I11], S1
// 
//  5 digit palindromes
//    S0 = M100
//    S1 = m:[S0,I110],S0
//    sequence = I2, q:[S1, I11], S1
// 
//  4 digit palindromes
//    S0 = M110
//    sequence = I2, q:[S0,I11], S0
//
//  3 digit palindromes
//    S0 = M10
//    sequence = I2, q:[S0,I11], S0
//
//  2 digit palindromes
//    init: 11 (not actually in the list of generated palindromes)
//    sequence = q:M11
// 
// Finally, let's generalize for k
//
//  2k+1 digit palindromes (odd)
//    S0 = M(1 followed by k 0s)
//    A0 = I(11 followed by k-1 0s)
//    S1 = m:[ S0, A0 ], S0
//    A1 = A0 >> 1  <-- 11 followed by k-2 0s
//    S2 = m:[ S1, A1 ], S1 ]
//    A2 = A1 >> 1
//    ..
//    Sk = q:[S(k-1), A(k-1)], S(k-1)
//    sequence = I2, Sk
//
//  2k+2 digit palindrome (even)
//    S0 = M(11 followed by k 0s)
//    A0 = I(11 followed by k-1 0s)
//    S1 = m:[ S0, A0 ], S0
//    A1 = A0 >> 1  <-- 11 followed by k-2 0s
//    S2 = m:[ S1, A1 ], S1 ]
//    A2 = A1 >> 1
//    ..
//    Sk = q:[S(k-1), A(k-1)], S(k-1)
//    sequence = I2, Sk
//-------------------------------------------------------------------------------------------------
