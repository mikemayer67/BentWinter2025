# BentWinter2025
Solution attempts for the computer bonus for the Winter 2025 computer bonus

# Problem Statement

A number in a particular base is palindromic if the digits (no leading zeros allowed) read the same right to 
left as left to right. For each integer BASE N > 2, let P(N) be the decimal representation of the smallest 
integer exceeding 2N that is palindromic both in base N and in base 2.
Consider the sequence of numbers formed by including P(N) (as N is incremented by 1) provided it is larger 
than any existing P(N) already in the sequence. To wit, P(3), which can be represented as 100010001 in base 3 
and 1100111110011 in base 2 (and 6643 as a decimal) is established as the first number. P(4) which can be 
represented as 33 in base 4 and 1111 in base 2 (and 15 as a decimal), is smaller than P(3) and, therefore, 
excluded from the sequence.

- What is the first decimal numer in this sequence larger than one quadrillion?
- What is the corresponding N and which number in the sequence is it?

# Solution

This took me 4 attempts to finally get a working solution:

1. Brute force in python.  Generate Base-N palindromes as a string of digits, Convert to binary.  Check if it was a palindrome.  (After 12 minutes, P(N) was only in the 10’s of millions and it was taking longer and longer between subsequent values).   WAY TOO SLOW … as in years or decades.

3. Ok… python is slow, try recoding it C++.  I got P(N) to a million in 3 minutes and a billion in an hour.   Nope.

4. Perhaps it was all the allocation/deallocation of memory.   One more recode.  This time in pure C using global buffers to avoid dynamic memory allocation.  I got to a million in just under 3 minutes and a billion in 50 minutes.

5. "And now for something completely different."  Noting that there wasn't much of a time difference between C and C++, 
I went back to C++ for all the convenience it brings relative to pure C.   **BUT**, I did a major rethink about how to go
about solving it.  There is plenty of commentary in the file itself, so I won't repeat it here.  Bottom line is that I
stopped focusing on strings of digits and switched to focusing on bits (*a single unsigned 64 bit integer is more than
sufficient to hold 1 quadrillion*).  Also I came up with a pseudo-turing machine for generating the palindromes without
needing to consider the digits themselves.

