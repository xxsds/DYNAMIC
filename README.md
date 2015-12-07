DYNAMIC: a succinct and compressed dynamic data structures library
===============
Author: Nicola Prezza
mail: nicola.prezza@gmail.com

### Brief introduction

This library offers space- and time-efficient implementations of some basic succinct/compressed dynamic data structures. Note that at the moment the library does not feature delete operations (only inserts). DYNAMIC features:

- A succinct Searchable Partial Sums with Inserts (SPSI) structure. Space: about 1.3 * n * k bits (n integers of bit-size k). The structure supports also update operations (i.e. modify internal elements).
- A Succinct dynamic bitvector supporting rank/select/access/insert (RSAI) operations. Space: about 1.1 * n bits.
- A gap-compressed dynamic bitvector supporting RSAI operations. Space: about 1.3 * b * log(n/b) bits,  where b is the number of bits set. All operations take log(b) time.
- A dynamic string supporting RSAI operations. The user can choose at construction time between fixed-length/gamma/Huffman encoding of the alphabet. All operations take log(n) * log(sigma) time (or log(n) * H0 with Huffman encoding).
- A run-length encoded dynamic string supporting RSAI operations. Space: approximately R*(1.1 * log(sigma) + 2.6 * log(n/R)) bits, where R is the number of runs in the string. All operations take log(R) time.

The SPSI structure is the building block on which all other structures are based. This structure is implemented with cache-efficient B-trees.

TODO: implement delete operations!

### Download

> git clone --recursive https://github.com/nicolaprezza/dynamic

### Compile

To compile, firstly create and enter a bin/ directory

> mkdir bin; cd bin

Then, launch cmake as (default build type is release):

> cmake ..

Finally, build the executables:

> make

The above command creates the executables in the bin directory.
