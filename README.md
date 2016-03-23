DYNAMIC: a succinct and compressed dynamic data structures library
===============
Author: Nicola Prezza
mail: nicola.prezza@gmail.com

### Data structures

This library offers space- and time-efficient implementations of some basic succinct/compressed dynamic data structures. Note that at the moment the library does not feature delete operations (only inserts). DYNAMIC features:

- A succinct Searchable Partial Sums with Inserts (SPSI) structure. Space: about 1.3 * n * k bits (n integers of bit-size k). The structure supports also update operations (i.e. modify internal elements).
- A Succinct dynamic bitvector supporting rank/select/access/insert (RSAI) operations. Space: about 1.1 * n bits.
- A gap-compressed dynamic bitvector supporting RSAI operations. Space: about 1.3 * b * log(n/b) bits,  where b is the number of bits set. All operations take log(b) time.
- A dynamic sparse vector (of integers).
- A dynamic string supporting RSAI operations. The user can choose at construction time between fixed-length/gamma/Huffman encoding of the alphabet. All operations take log(n) * log(sigma) time (or log(n) * H0 with Huffman encoding).
- A run-length encoded dynamic string supporting RSAI operations. Space: approximately R*(1.1 * log(sigma) + 2.6 * log(n/R)) bits, where R is the number of runs in the string. All operations take log(R) time.
- A dynamic (left-extend only) entropy/run-length compressed BWT
- A dynamic (left-extend only) entropy/run-length compressed FM-index. This structure consists in the above BWT + a dynamic suffix array sampling

### Algorithms

- Two algorithms to build LZ77 in repetition-aware RAM working space. Both algorithms use a run-length encoded BWT with sparse Suffix array sampling. The first algorithm stores 2 SA samples per BWT run. The second algorithm (much more space efficient) stores 1 SA sample per LZ factor. From the paper "Computing LZ77 in Run-Compressed Space", Alberto Policriti and Nicola Prezza, DCC2016
- An algorithm to build the BWT in run-compressed space
- An algorithm to build LZ77 in nH0(1+o(1)) space and n * log n * H0 time. From the paper "Fast Online Lempel-Ziv Factorization in Compressed Space", Alberto Policriti and Nicola Prezza, SPIRE2015

The SPSI structure is the building block on which all other structures are based. This structure is implemented with cache-efficient B-trees.

### TODO: 

- Implement delete operations
- Implement a good memory allocator. At the moment the default allocator is used, which results in about 25% of memory being wasted due to fragmentation
- Geometric data structures (predecessor/2D range search)

### Download

> git clone https://github.com/nicolaprezza/dynamic

### Compile

Thre library features some example executables. To compile them, firstly create and enter a bin/ directory

> mkdir bin; cd bin

Then, launch cmake as (default build type is release):

> cmake ..

Finally, build the executables:

> make

The above command creates the executables in the bin directory. 

### Usage

The header include/dynamic.hpp contains all type definitions and is all you need to include in your code. The folder algorithms/ contains some algorithms implemented with the library's structures. This is a snapshot of dynamic.hpp:

    /*
     * a succinct searchable partial sum with inserts implemented with cache-efficient
     * B trees.
     */
    typedef spsi<packed_vector,256,16> packed_spsi;

    /*
     * dynamic gap-encoded bitvector
     */
    typedef gap_bitvector<packed_spsi> gap_bv;

    /*
     * dynamic succinct bitvector (about 1.1n bits)
     */
    typedef succinct_bitvector<spsi<packed_vector,8192,16> > suc_bv;

    /*
     * succinct/compressed dynamic string implemented with wavelet trees.
     * user can choose (at construction time) between fixed-length / gamma / Huffman encoding of characters.
     */
    typedef wt_string<suc_bv> wt_str;

    /*
     * run-length encoded (RLE) string. This string uses 1 sparse bitvector
     * for all runs, one dynamic string for run heads, and sigma sparse bitvectors (one per character)
     */
    typedef rle_string<gap_bv, wt_str> rle_str;

    /*
     * RLE string implemented with a run-length encoded wavelet tree. Each
     * WT node is run-length encoded. 
     */
    typedef wt_string<rle_str> wtrle_str;

    /*
     * succinct/compressed BWT (see description of wt_str)
     */
    typedef bwt<wt_str,rle_str> wt_bwt;

    /*
     * run-length encoded BWT
     */
    typedef bwt<rle_str,rle_str> rle_bwt;

    /*
     * dynamic sparse vector: <= m*k + O(m log n/m) bits of space, where k is the maximum
     * number of bits of any integer > 0 and n is the total number of integers.
     */
    typedef sparse_vector<packed_spsi,gap_bv> sparse_vec;

    /*
     * dynamic succinct/entropy compressed FM index. BWT positions are
     * marked with a succinct bitvector
     *
     * ( n*H0 + n + (n/k)*log n )(1+o(1)) bits of space, where k is the SA sample  rate
     *
     */
    typedef fm_index<wt_bwt, suc_bv, packed_spsi> wt_fmi;

    /*
     * dynamic run-length encoded FM index. BWT positions are
     * marked with a gap-encoded bitvector.
     *
     * ( 2*R*log(n/R) + R*H0 + (n/k)*log(n/k) + (n/k)*log n )(1+o(1)) bits of  space, where
     * k is the SA sample rate and R is the number of runs in the BWT
     *
     */
    typedef fm_index<rle_bwt, gap_bv, packed_spsi> rle_fmi;
