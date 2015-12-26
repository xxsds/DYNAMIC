/*
 * rle_lz77_v2.hpp
 *
 *  Created on: Dec 17, 2015
 *      Author: nico
 *
 *  Compute the LZ77 parsing in run-compressed space
 *  using a dynamic run-length encoded BWT with
 *  a sparse SA sampling (1 sample per LZ factor).
 *
 *  Space is O(R log n + z log n) bits; the constants hidden in the
 *  big-O notation are much smaller than in rle_lz77_v1.hpp
 *
 *  Type of input text here is uchar
 *
 *
 */

#ifndef INCLUDE_ALGORITHMS_RLE_LZ77_V2_HPP_
#define INCLUDE_ALGORITHMS_RLE_LZ77_V2_HPP_

#include <dynamic.hpp>

namespace dyn{

class rle_lz77_v2{

public:

	using char_t = rle_bwt::char_type;

	/*
	 * Constructor #1: run-heads are gamma-coded
	 */
	rle_lz77_v2(){}

	/*
	 * Constructor #2
	 *
	 * We know only alphabet size. Each Run-head char is assigned log2(sigma) bits.
	 * Characters are assigned codes 0,1,2,... in order of appearance
	 *
	 */
	rle_lz77_v2(uint64_t sigma){

		assert(sigma>0);
		RLBWT = rle_bwt(sigma);

	}

	/*
	 * Constructor #3
	 *
	 * The constructor scans the input once and computes
	 * characters probabilities. These probabilities are
	 * used to Huffman-encode run heads. The stream used here
	 * should be the same used in parse(istream& in, ostream& out)
	 * (but it is not necesssary)
	 *
	 * Here Run-heads are Huffman encoded.
	 *
	 */
	rle_lz77_v2(istream& in){

		auto freqs = get_frequencies(in);
		RLBWT = rle_bwt(freqs);

	}

	/*
	 * input: an input stream and an output stream
	 * the algorithms scans the input (just 1 scan) and
	 * saves to the output (could be a file) a series
	 * of triples <pos,len,c> of type <ulint,ulint,uchar>
	 */
	void parse(istream& in, ostream& out, bool verbose = false){

		/* last position (on text) of current LZ phrase prefix.
		 * at the beginning, j=1: we ignore the BWT terminator
		 */
		ulint j = 1;
		ulint k = 0;			/* position in RLBWT corresponding to position 1 in the text*/
		ulint l = 0;			/* Length of current LZ phrase prefix */

		pair<ulint, ulint> range = {0,0};	/* range of current LZ phrase prefix
											 * full interval is <0,n> : intervals
											 * are of the form [l,r)
											 */

		char c;
		while(in.get(c)){

			/* extend search */

			range = RLBWT.LF(range,c);

			if(range.second <= range.first){

				/* empty range: end of a LZ factor */



			}else{


			}

			RLBWT.extend( char_t(c) );

		}




	}

private:

	//the run-length encoded BWT
	rle_bwt RLBWT;

	//suffix array samples. These are pointers to the bits set in rep
	sparse_vec SA;

	/*
	 * a BWT position can be shared as source by more than 1 LZ factor.
	 * If SA[i] != NIL, then ptr[ rep.select1( SA.rank(i) ), ..., rep.select1( SA.rank(i) +1)-1 ]
	 * are pointers to LZ factors that share the BWT position i as source.
	 */
	suc_bv rep;
	packed_spsi ptr;

	/* the LZ factors. This vector stores 3z integers:
	 *
	 *  factors[3*i+0] = start position of i-th factor
	 * 	factors[3*i+1] = length of i-th factor (minus 1: trailing char is excluded)
	 * 	factors[3*i+2] = trailing character of i-th factor
	 */
	packed_spsi factors;

};

}

#endif /* INCLUDE_ALGORITHMS_RLE_LZ77_V2_HPP_ */
