//
//  main.cpp
//  CompBonus
//
//  Created by Mike Mayer on 1/10/25.
//

#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>

// Problem Statement
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
//
// Observations
//
// - The number must be odd as the binary representation of an even number will end in 0 
//     and thus cannot be palindromic without a leading 0
//
// - The smallest possible integer exceeding 2N will always be 21 regardless of base. Thus 
//     22 will always be the smallest palindrome in base N.
//
// Design Notes
//
// - A "digit" is an integer value in the range [0,N)
//   - each is represented by the normal symbols: 0, 1, ..., 9, A, B, ..., C
//   - if N needs to go above 36, we will need to extend this->list
// - A "number" is a little-endian vector of digits
//   - does not have an implicit base
//   - is only valid if the largest digit is less than the base
// - A "bitstring" is a list of binary digits

typedef uint8_t                     Bit_t;
typedef uint64_t                    Digit_t;
typedef std::vector<Digit_t>        Number_t;
typedef std::vector<Bit_t>          Binary_t;

class Base
{
  // Provides base specific arithmetic functions as needed for the computer bonus

  public:
    Base(Digit_t base) : _N(base), _odd(base % 2 == 1)
    {
      // As division and modulus are expensive from a runtime POV and as one of the expected
      //   bottlenecks will be converting number to binary, we are going to precompute the
      //   quotient and remainder when dividing any number in the range 0 to 2N-1 by 2.
      // To improve retrieval efficiency, the will be stored in an array of 2 vectors:
      //   0 to  N-1 will be stored in the first row  (single digit base N numbers)
      //   N to 2N-1 will be stored in the second row (two digit base N numbers where first digit is 1)
      for(Digit_t i=0; i<2*base; ++i)
      {
        _Q2[i/base].push_back(i/2);
        _R2[i/base].push_back(i%2);
      }
      
      // In order preallocate arrays of bits when converting to binary, it is useful to have
      //   an upper bound on the number of bits to expect for an M digit base N number.
      //   - any M digit base N number will be less than N^M
      //   - this->can be encoded with a B bit binary number if
      //        2^(B-1) >= N^M
      //        (B-1) log(2) >= M log(N)
      //        B >= 1 + M log(N)/log(2)
      //        B = ceil(1 + M log2(N))
      this->_log2N = std::log(base) / std::log(2.0);
    }
  
    Digit_t N() const { return _N; }

    bool divmod2(Number_t &n) const
    {
      // This function divides the input number by 2
      // Note that this modifies the input number in place
      // It returns a boolean indicating if there was or was a remainder (i.e. odd or even)
      bool R = false;
      auto dit = n.rbegin();
      if(*dit < 2) {
        R = true;
        *(dit++) = 0;
      }
      Digit_t Q(0);
      while( dit != n.rend() ) {
        Q = _Q2[R][*dit];
        R = _R2[R][*dit];
        *(dit++) = Q;
      }
      if(n.back() == 0) {
        n.pop_back();
      }
      return R;
      }
    
    Digit_t divmod(Number_t &n,Digit_t base) const
    {
      // This function divides the input number by input divisor
      //   Note that this modifies the input number in place
      //   It returns the remainder
      // This function is essentially identical to divmod2 except that
      //   where divmod2 is called many many times while trying to find P[N],
      //   divmod will only be called once P[N] has been found and
      //   we need to convert it to a an other base.  There is no need
      //   to precompute the q,r pairs for arbitrary base
      Digit_t Q = 0;
      Digit_t R = 0;
      auto dit = n.rbegin();
      while( dit != n.rend() ) {
        Digit_t nn = _N * R + *dit;
        Q = nn / base;
        R = nn - base*Q;
        *(dit++) = Q;
      }
      while(!n.empty() && n.back() == 0) {
        n.pop_back();
      }
      return R;
    }
  
    Binary_t binary_if_palindrome(const Number_t &n) const
    {
      // Converts the input number to binary, but only if palindrome
      
      // Quick check... if n is even, the binary cannot be a palindrome
      //  as this would require the leading bit to also be a zero
      //  for odd bases: number is odd if sum off all digits is odd
      //  for even bases: number is odd if the least signifcant digit is odd
      Digit_t parity = 0;
      if( _odd ) {
        for(auto dit = n.begin(); dit != n.end(); ++dit) {
          parity += *dit;
        }
      } else {
        parity = n.front();
      }
      if(parity % 2 == 0) { return Binary_t(); }
      
      Binary_t bits = binary(n);
      
      // check for palindrome (itsif applicable)
      uint16_t nbits = bits.size();
      for(uint16_t i=0; i<nbits/2; ++i) {
        if ( bits[i] != bits[nbits-1-i] ) {
          bits.clear();
          break;
        }
      }
      
      return bits;
    }
  
    Binary_t binary(const Number_t &n, bool only_palindrome=false) const
    {
      // Converts the input number to binary.
      Binary_t bits;
      
      // as divmod2 modifies the number passed it, we need a non-const
      //   copy of n to work with
      Number_t digits(n);
      
      // to avoid unnecessary resizing, let's reserve enough space in the
      //   return value (bits) to hold all the binary digits
      bits.reserve(1 + ceil(digits.size() * _log2N));
    
      // create (little-endian) binary number by repeatedly
      //   dividing n by 2 and recording the remainder until n=0
      while(!digits.empty()) {
        bits.push_back( divmod2(digits) );
      }
      
      return bits;
    }

  Number_t convert(const Number_t &n, Digit_t base) const
  {
    // Converts the input number to the specified base.
    Number_t rval;
    
    // as divmod modifies the number passed it, we need a non-const
    //   copy of n to work with
    Number_t digits(n);
    
    // to avoid unnecessary resizing, let's reserve enough space in the
    //   return value (bits) to hold all the binary digits
    rval.reserve(1 + ceil(digits.size() * log(_N)/log(base)));
    
    // create (little-endian) binary number by repeatedly
    //   dividing n by base and recording the remainder until n=0
    while(!digits.empty()) {
      rval.push_back( divmod(digits,base) );
    }
    
    return rval;
  }
    
  std::string decimal(const Number_t &n) const
  {
    Number_t n10 = convert(n,10);
    std::stringstream rval;
    auto nd = n10.size();
    for(auto dit=n10.rbegin(); dit!=n10.rend(); ++dit) {
      rval << int(*dit);
      --nd;
      if( nd>0 && (nd%3 == 0) ) { rval << ","; }
    }
    return rval.str();
  }

  private:
    Digit_t              _N;     // the base
    bool                 _odd;   // flag indicating that the base is odd
    std::vector<Digit_t> _Q2[2]; // see description in constructor
    std::vector<bool>    _R2[2]; // see description in constructor
    double               _log2N; // see description in constructor
};

std::ostream &operator<<(std::ostream &s, const Binary_t &bits)
{
  for (auto bit=bits.rbegin(); bit!=bits.rend(); ++bit) {
    s << int(*bit);
  }
  return s;
}

std::ostream &operator<<(std::ostream &s, const Number_t &n)
{
  auto dit = n.rbegin();
  s << *dit;
  for (++dit; dit!=n.rend(); ++dit) {
    s << "|" << int(*dit);
  }
  return s;
}

bool operator>(const Binary_t &a, const Binary_t &b)
{
  // compares two binary numbers (a and b) and returns true if a>b
  //   it is assumed that a and b are of the same base
  if(a.size() > b.size()) { return true;  }
  if(a.size() < b.size()) { return false; }
  auto ait = a.rbegin();
  auto bit = b.rbegin();
  for(; ait!=a.rend(); ++ait, ++bit)
  {
    if( *ait > *bit) { return true; }
    if( *ait < *bit) { return false; }
  }
  return false;  // equal
}

class Palindromes
{
  // Generates a sequence of Base-N palindromes (think python generator)
  //   The smallest palindrome greater than 2N in any base (>2) is 22.
  //   (11 would be N+1, which is less than 2N)
  // Consider a palindrome with 2m digits.  It can be written as:
  //    d(m-1), d(m-2), d(m-3), ..., d1, d0, d0, d1, ..., d(m-2), d(m-1)
  //    with a distinct "kernel": d0, d1, d2, ..., d(m-1)
  // Similarly a palindrom with 2m-1 digits, can be written as:
  //    d(m-1), d(m-2), d(m-3), ..., d1, d0, d1, ..., d(m-2), d(m-1)
  //    with the same "kernel": d0, d1, d2, ..., d(m-1)
  // Note that the kernel is stored in the back half of the palindrome.
  //
  // Generation of the sequence boils down to:
  // - starting with m=1, examine increasing values of m
  // - with each value of m>1, generate all palindromes of length 2m-1 and
  //     then all palindromes of length 2m
  //     (for  m=1, start with the palindromes of length 2m)
  // - for any given palindrom length, start with kernel = 0000...1
  //     (i.e., d(m-1)=1, all other di=0)
  // - increment kernel by one until all digits are equal to N-1
  //     increment d0 by 1
  //     if d0 is now N, set it to 0 and increment d1
  //     if d1 is now N, set it to 0 and increment d2
  //     repeat for all di up to d(m-2)
  // - if d(m-1) is now N, we've exhaused all kernels.  move to next length palindrome
  //     if just finished odd palindrome length (2m-1 digits), go on to 2m digits
  //     if just finished even length, increment m and start with odd length
  //
  //--------------------------------------------------------------------
  // Refer to the following graphic to understand how this looks:
  //--------------------------------------------------------------------
  //   X = largest digit in base N
  //   a = index of digit in kernel portion of palindrome being updated
  //   b = index of mirror digit (not in kernel)
  //
  //  Starting state:
  //               |<-kernel->|
  //      1 0 0 0 0|0 0 0 0 1
  //              b|a
  //  Subsequent states up to first digit rollover:
  //      1 0 0 0 1|1 0 0 0 1   (increment current digit)
  //      1 0 0 0 2|2 0 0 0 1
  //      1 0 0 0 3|3 0 0 0 1
  //              ...
  //      1 0 0 0 X|X 0 0 0 1
  //  First rollover of first digit:
  //              b|a
  //      1 0 0 0 0|0 0 0 0 1   (clear current digit and advance to next)
  //            b  |  a
  //      1 0 0 1 0|0 1 0 0 1   (increment current digit)
  //  Subsequent states up to rollover of first two digits:
  //      1 0 0 1 1|1 1 0 0 1   (increment current digit)
  //      1 0 0 1 2|2 1 0 0 1
  //      1 0 0 1 3|3 1 0 0 1
  //              ...
  //      1 0 0 1 X|X 1 0 0 1
  //      1 0 0 2 0|0 2 0 0 1
  //      1 0 0 2 1|1 2 0 0 1
  //              ...
  //      1 0 0 X X|X X 0 0 1
  //  Rollover of first two digits:
  //              b|a
  //      1 0 0 X 0|0 X 0 0 1   (clear current digit and advance to next)
  //            b  |  a
  //      1 0 0 0 0|0 0 0 0 1   (clear current digit and advance to next)
  //          b    |    a
  //      1 0 1 0 0|0 0 1 0 0   (increment current digit)
  //  Subsequent states up to final valid state with 2m digits:
  //      1 0 1 0 1|1 0 1 0 0
  //      1 0 1 0 2|2 0 1 0 0
  //              ...
  //      X X X X X|X X X X X
  //
  //  Starting state of next sequence
  //               a
  //     1 0 0 0 0 0 0 0 0 0 1  (increment palindrome length, reset all digits)
  //               |
  //  Subsequent states up to first digit rollover:
  //               a
  //     1 0 0 0 0 1 0 0 0 0 1   (increment current digit)
  //     1 0 0 0 0 2 0 0 0 0 1
  //     1 0 0 0 0 3 0 0 0 0 1
  //              ...
  //     1 0 0 0 0 X 0 0 0 0 1
  //  First rollover of first digit:
  //               a
  //     1 0 0 0 0 0 0 0 0 0 1  (clear current digit and advance to next)
  //             <-|->
  //     1 0 0 0 1 0 1 0 0 0 1  (increment current digit)
  //             b | a
  //  Subsequent states up to the final state with 2m-1 digits:
  //     1 0 0 0 1 1 1 0 0 0 1
  //     1 0 0 0 1 2 1 0 0 0 1
  //              ...
  //     1 0 0 0 2 0 2 0 0 0 1
  //     1 0 0 0 2 1 2 0 0 0 1
  //              ...
  //     1 0 0 0 X X X 0 0 0 1
  //     1 0 0 1 0 0 0 1 0 0 1
  //     1 0 0 1 0 1 0 1 0 0 1
  //              ...
  //     X X X X X X X X X X X
  //--------------------------------------------------------------------
  // Starting state
  //    11   (initialized value...)
  //    22   (first returned value)
  //    33
  //    ...
  //    XX
  //    101
  //    111
  //    121
  //    ...
  //    1X1
  //    202
  //    212
  //    ...
  //    2X2
  //    303
  //    313
  //    ...
  //    XXX
  //    1001
  //    1111
  //    1221
  //    ...
  //    XXXX
  //    10001
  //    11111
  //    ...
  //--------------------------------------------------------------------
  
public:
  // we seed the sequence with 11 so that the first call to next will yield 22
  Palindromes(Digit_t base) : _N(base), _n(2), _cur({1,1})
  {}
  
  const Number_t &next()
  {
    // returns the next palindrome

    // Initialize a and b based on the current palindrom length
    // n: 2 3 4 5 6 7 8 9
    // m: 1 2 2 3 3 4 4 5   (n+1)/2
    // a: 1 1 2 2 3 3 4 4   n/2
    // b: 0 1 1 2 2 3 3 4   m-1
    uint32_t a = _n/2;
    uint32_t b = (_n-1)/2;
    
    while( a < _n ) {
      if( _cur[a] < _N-1 ) {
        _cur[a] = _cur[b] = 1 + _cur[a];
        break;
      } else {
        _cur[a] = _cur[b] = 0;
        a = a + 1;
        b = b - 1;
      }
    }
    
    // see if we need to increase the number of digits
    if( a == _n ) {
      for( auto dit=_cur.begin(); dit!=_cur.end(); ++dit) { *dit=0; }
      _cur[0] = 1;
      _cur.push_back(1);
      _n = _n + 1;
    }

    return _cur;
  }
  
private:
  Digit_t  _N;    // numeric base for the palindrome
  uint32_t _n;    // crurrent length of the palindrome
  Number_t _cur;  // current palindrome
};


Number_t P(Base &base)
{
  // computes the value of P(N)
  Palindromes p(base.N());
  while(true) {
    const Number_t &n = p.next();
    Binary_t bits = base.binary_if_palindrome(n);
    if(!bits.empty()) {
      return n;
    }
  }
}



int main(int argc, char **argv)
{
  Binary_t max;
  
  uint32_t nfound=0;
  
  for(Digit_t i=3; i>1; ++i) {
    Base base(i);
    Number_t pn = P(base);
    Binary_t bn = base.binary(pn);
    if( bn > max ) {
      max = bn;
      std::cout << ++nfound << ": " << i << " " << pn << " " << base.decimal(pn) << " " << base.binary(pn) << std::endl;
    }
  }
}
