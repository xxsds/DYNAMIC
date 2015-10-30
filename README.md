DYNAMIC: a succinct and compressed dynamic data structures library
===============
Author: Nicola Prezza
mail: nicolapr@gmail.com

### Brief introduction

This library offers space- and time-efficient implementations of dynamic structures such as:

- A succinct Searchable Partial Sums with Inserts (SPSI) structure. Space: about 1.3 nk bits (n integers of bit-size k)
- A Succinct dynamic bitvector with rank/access/select/insert. Space: about 1.1n bits
- A gap-compressed dynamic bitvector with rank/access/select/insert. 

The SPSI structure is the building block on which all other structures are based. This structure is implemented with B-trees, which makes it very cache-efficient and light. 

All operations are supported in O(log n) time.

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
