#------------------------
#  NOTE THAT THIS ATTEMPT WAS A FAILURE
#------------------------
#  I gave up before going to completion when I realized how long it was taking just
#  to generate the Base-N palindromes... and the fact that I was going to run out of symbols for the digits
#------------------------

#------------------------
# Problem Statement
#------------------------
# A number in a particular base is palindromic if the digits (no leading zeros allowed) read the same right to 
#  left as left to right. For each integer BASE N > 2, let P(N) be the decimal representation of the smallest 
#  integer exceeding 2N that is palindromic both in base N and in base 2.
# Consider the sequence of numbers formed by including P(N) (as N is incremented by 1) provided it is larger 
#  than any existing P(N) already in the sequence. To wit, P(3), which can be represented as 100010001 in base 3 
#  and 1100111110011 in base 2 (and 6643 as a decimal) is established as the first number. P(4) which can be 
#  represented as 33 in base 4 and 1111 in base 2 (and 15 as a decimal), is smaller than P(3) and, therefore, 
#  excluded from the sequence.
#
# What is the first decimal numer in this sequence larger than one quadrillion?
# What is the corresponding N and which number in the sequence is it?

#------------------------
# Observations
#------------------------
# The number must be odd as the binary representation of an even number will end in 0... and thus cannot be palindromic without a leading 0
# The smallest possible integer exceeding 2N will always be 21 regardless of base. Thus 22 will always be the smallest palindrome in base N.

#------------------------
# Design Notes
#------------------------
# A "digit" is an integer value in the range [0,N)
# each is represented by the normal symbols: 0, 1, ..., 9, A, B, ..., C
# if N needs to go above 36, we will need to extend this list
# A "number" is a big-endian list of digits
# does not have an implicit base
# is only valid if the largest digit is less than the base
# "bits" is a list of binary digits


_digit_map = {i:d for i,d in enumerate("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ")}

def number_to_string(n):
    # n is a number
    # returns a string representing the number in human readable form
    # this function is base agnostic
    return "".join((_digit_map[d] for d in n))

class Base:
    # provides the base specific arithmetic functions as needed
    
    def __init__(self,N):
        self.N = N
        self.odd = N % 2

        # As division and modulus is expensive from a runtime POV and as we
        #   will only ever need to divide by 2 (for conversion to bits), we
        #   are going to precompute 1 or 2 digits cases we will encounter.
        # Specifically, we will precompute quotient and remainder for all
        #   number in the range 0 to 2N-1:
        #     0 to  N-1: [0,d] where d is in range 0 to N-1
        #     N to 2N-1: [1,d] where d is in range 0 to N-1
        # Values will be stored as a map from n to (q,r) where:
        #   n is a 2 digit sequence of the form (0,d) or (1,d)
        #   q is the quotient from dividing by 2 (will be in the range 0 to N-1)
        #   r is the remainder from dividing by 2 (will be either 0 or 1)
        self._qr2 = {divmod(n,N):divmod(n,2) for n in range(2*N)}

        # @@@ don't delete this yet... i might come back to it
        # In order to preallocate the array that will store bits when
        #   converting to binary, it is useful to have an upper bound on
        #   the number of bits to expect for an M digit base-N number.
        #     - the value of the number will be less than N^M
        #     - we need, therefore, enough bits (B) to represent N^M
        #          2^(B-1) >= N^M
        #          (B-1) log(2) >= M log(N)
        #          B >= 1 + M log(N)/log(2)
        #          B = ceil(1 + M log2(N))
        # self.log2N = np.log(N)/np.log(2)

    def __str__(self):
        return f"Base({self.N})"

    def divmod2(self,n):
        # n is a number
        # returns Q,R where:
        #   Q is the quotient (a base N number) when n is divided by 2
        #   R is the remainder (0 or 1) when n is divided by 2
        assert n[0] > 0, "should never be called with a leading 0: " + number_to_string(n)

        if n[0] == 1:
            # first digit is not divisible by 2, move on to dividing first 2 digits
            R = 1
            n = n[1:]
        else:
            # first digit is divisible by 2
            R = 0

        Q = list()
        for i,d in enumerate(n):
            q,R = self._qr2[(R,d)]
            Q.append(q)

        return Q,R

    def divmod10(self,n):
        # n is a number
        # returns Q,R where:
        #   Q is the quotient (a base N number) when n is divided by 10
        #   R is the remainder (0 or 1) when n is divided by 10
        #
        # This function is essentially identical to divmod2 except that
        #   where divmod2 is called many many times while trying to find P[N],
        #   divmod10 will only be called once P[N] has been found and
        #   we need to convert it to a decimal value.  There is no need
        #   to precompute the q,r pairs for division by 10
        assert n[0] > 0, "should never be called with a leading 0: " + number_to_string(n)

        R = 0
        Q = list()
        for i,d in enumerate(n):
            q,R = divmod(R*self.N + d, 10)
            if q or Q:  # avoids leading zeros
                Q.append(q)
        return Q,R


    def bits(self,n):
        # n is a number
        # returns the binary number if and only if it is a palindrome
        #   otherwise returns None

        # quick check... if n is an even number, the binary cannot be a palindrome
        #   as this would require it to start with a leading 0
        #   for odd bases: number is odd if sum of all digits is odd
        #   for even bases: number is odd if final digit is odd
        parity = sum(n)  if self.odd else n[-1]
        if parity % 2 == 0:
            return None

        # create the (little-endian) list of binary digits by continuously
        #  dividing by 2 and recording the remainders
        bits = list()
        while n:
            n,r = self.divmod2(n)
            bits.append(r)

        # check bits for palindrome
        #   we don't care that it's little-endian
        nbits = len(bits)
        for i in range(nbits//2):
            if bits[i] != bits[nbits-1-i]:
                return None
                
        # we have a palindromic binary number, return it
        #   no need to convert to big-endian as it's a palindrome
        return bits

    def decimal(self,n):
        # n is a number
        # returns the decimal number as a string

        # create the (little-endian) list of decimal digits by continuously
        #  dividing by 10 and recording the remainders
        decimal = list()
        while n:
            n,r = self.divmod10(n)
            decimal.append(r)
        # flip to big-endian and convert to number string
        decimal.reverse()
        decimal = number_to_string(decimal)
        return decimal

    def palindromes(self,length):
        # generator function for returning all of the palindromes
        #   of given length with constraint that minimum is 2N

        # determine if length is odd (we'll need this soon)
        odd = length % 2
        
        # determine number of digits in half of the palindrome
        #   odd length palindromes have an "extra" digit in the center
        m = (1+length)//2 if odd else length//2

        # if length is 2, then smallest half-palindrome is 2 (to avoid 11, which is < 2N)
        # otherwise, the smallest half-palindrome is 1 with m-1 following zeros
        n = [2] if length == 2 else [1]+[0 for _ in range(m-1)]

        while n[0] < self.N:
            # generate palindrome
            if odd:
                yield n + list(reversed(n[:-1]))
            else:
                yield n + list(reversed(n))
            # advance to next
            for i in range(m-1,-1,-1):
                n[i] = 1 + n[i]
                if n[i] < self.N:
                    break
                if i:
                    n[i] = 0

    def first_palindrome(self):
        for p_len in range(2,30):
            for n in self.palindromes(p_len):
                bits = self.bits(n)
                if bits:
                    return (self.N, n, bits, self.decimal(n) )

    def from_binary(self,bits):
        # (only needed for dev/test purpose)
        # bits is a binary number (big-endian)
        # returns a number in base N
        n = [0]
        for b in bits:
            n = [2*d for d in n]
            n[0] = n[0] + b
            C = 0
            for i,d in enumerate(n):
                C,d = divmod(d+C,self.N)
                n[i] = d
            if C:
                n.append(C)
        n.reverse()
        return n

    def from_decimal(self,decimal):
        # (only needed for dev/test purpose)
        # decimal is a base 10 number (big-endian)
        # returns a number in base N
        n = [0]
        for d in decimal:
            n = [10*t for t in n]
            n[0] = n[0] + d
            C = 0
            for i,t in enumerate(n):
                C,t = divmod(t+C,self.N)
                n[i] = t
            while C:
                C,t = divmod(C,self.N)
                n.append(t)
        n.reverse()
        return n

Pn = [Base(n).first_palindrome() for n in range(3,1000)]
print(Pn)