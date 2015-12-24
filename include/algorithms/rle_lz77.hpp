/*
 * rle_lz77.hpp
 *
 *  Created on: Dec 17, 2015
 *      Author: nico
 *
 *  Compute the LZ77 parsing in run-compressed space
 *  using a dynamic run-length encoded BWT with
 *  a sparse SA sampling (2 samples per BWT run).
 *
 *  Type of input text here is uchar
 *
 *  From the paper: Alberto Policriti and Nicola Prezza, "Computing LZ77 in Run-Compressed Space"
 *
 */

#ifndef INCLUDE_ALGORITHMS_RLE_LZ77_HPP_
#define INCLUDE_ALGORITHMS_RLE_LZ77_HPP_

#include <dynamic.hpp>

namespace dyn{

class rle_lz77{

public:

	/*
	 * Constructor #1: run-heads are gamma-coded
	 */
	rle_lz77(){}

	/*
	 * Constructor #2
	 *
	 * We know only alphabet size. Each Run-head char is assigned log2(sigma) bits.
	 * Characters are assigned codes 0,1,2,... in order of appearance
	 *
	 */
	rle_lz77(uint64_t sigma){

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
	rle_lz77(istream& in){

		auto freqs = get_frequencies(in);
		RLBWT = rle_bwt(freqs);

	}

	/*
	 * input: an input stream and an output stream
	 * the algorithms scans the input (just 1 scan) and
	 * saves to the output (could be a file) a series
	 * of triples <pos,len,c> of type <ulint,ulint,uchar>
	 */
	void parse(istream& in, ostream& out){

		using char_t = rle_bwt::char_type;

		/*
		 * Step 1: build dynamic RLBWT of reverse stream
		 */

		{

			char c;
			while(in.get(c)) RLBWT.extend( char_t(c) );

		}

		/*
		 * initialize variables
		 */

		ulint n = RLBWT.size();	//size of BWT (terminators included)
		ulint j = 0;			//last position (on text) of current LZ phrase prefix
		ulint k = RLBWT.get_terminator_position();	//position of terminator in RLBWT
		ulint l = 0;			//Length of current LZ phrase prefix
		/*
		 * Previous occurrence of current LZ phrase prefix. Value 0 is undefined (NULL)
		 * since no phrase can start at position 0 (because position 0 on the text
		 * contains the BWT terminator.
		 */
		ulint p = 0;

		//RBT

		char_t c = RLBWT[k];	// current T character

		pair<ulint, ulint> range = {0,n-1};

		/*
		 * Step 2: start parsing
		 */

		for(ulint i=0;i<RLBWT.size();++i)
			cout << uchar(RLBWT[i]==RLBWT.get_terminator()?'#':RLBWT[i]);

		cout << endl;

	}

private:

	//the run-length encoded BWT
	rle_bwt RLBWT;

};

}

#endif /* INCLUDE_ALGORITHMS_RLE_LZ77_HPP_ */
