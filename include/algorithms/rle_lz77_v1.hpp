/*
 * rle_lz77_v1.hpp
 *
 *  Created on: Dec 17, 2015
 *      Author: nico
 *
 *  Compute the LZ77 parsing in run-compressed space
 *  using a dynamic run-length encoded BWT with
 *  a sparse SA sampling (2 samples per BWT run).
 *
 *  Space is O(R log n) bits. Be aware however that the constant hidden in
 *  the big-O notation is quite high: space is around 6Rlog n bits.
 *  See the algorithm rle_lz77_v2 for a more space-efficient parse.
 *
 *  Type of input text here is uchar
 *
 *  From the paper: Alberto Policriti and Nicola Prezza, "Computing LZ77 in Run-Compressed Space"
 *
 *
 */

#ifndef INCLUDE_ALGORITHMS_rle_lz77_V1_HPP_
#define INCLUDE_ALGORITHMS_rle_lz77_V1_HPP_

#include <dynamic.hpp>

namespace dyn{

class rle_lz77_v1{

public:

	using char_t = rle_bwt::char_type;

	/*
	 * Constructor #1: run-heads are gamma-coded
	 */
	rle_lz77_v1(){}

	/*
	 * Constructor #2
	 *
	 * We know only alphabet size. Each Run-head char is assigned log2(sigma) bits.
	 * Characters are assigned codes 0,1,2,... in order of appearance
	 *
	 */
	rle_lz77_v1(uint64_t sigma){

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
	rle_lz77_v1(istream& in){

		auto freqs = get_frequencies(in);
		RLBWT = rle_bwt(freqs);

	}

	/*
	 * input: an input stream and an output stream
	 * the algorithms scans the input (just 1 scan) and
	 * saves to the output stream (could be a file) a series
	 * of triples <pos,len,c> of type <ulint,ulint,uchar>. Types
	 * are converted to char* before streaming them to out
	 * (i.e. ulint to 8 bytes and uchar to 1 byte)
	 */
	void parse(istream& in, ostream& out, bool verbose = false){

		long int step = 1000000;	//print status every step characters
		long int last_step = 0;

		/*
		 * Step 1: build dynamic RLBWT of reverse stream
		 */

		{

			ulint j = 0;

			if(verbose) cout << "Building RLBWT ..." << endl;

			char c;
			while(in.get(c)){

				if(verbose){

					if(j>last_step+(step-1)){

						last_step = j;
						cout << " " << j << " characters processed ..." << endl;

					}

				}

				RLBWT.extend( uchar(c) );

				j++;

			}

		}

		/*
		 * initialize variables
		 */

		ulint n = RLBWT.size();	/* size of BWT (terminators included)  */

		/* last position (on text) of current LZ phrase prefix.
		 * at the beginning, j=1: we ignore the BWT terminator
		 */
		ulint j = 1;

		ulint k = 0;			/* position in RLBWT corresponding to position 1 in the text*/

		ulint l = 0;			/* Length of current LZ phrase prefix */
		ulint p = 0;			/* Previous occurrence of current LZ phrase prefix.
								 * Value 0 is undefined (NULL) since no phrase can
								 * start at position 0 (because position 0 on the text
								 * contains the BWT terminator.
								 */

		{

			//initialize Suffix array samples

			auto A = RLBWT.get_alphabet();	//get alphabet

			//for each character, create a sparse vector of SA samples
			for(auto c : A){

				SA[c] = sparse_vec(n);

			}

		}

		char_t c = RLBWT[k];	/* current T character */

		pair<ulint, ulint> range = {0,n};	/* range of current LZ phrase prefix
											 * full interval is <0,n> : intervals
											 * are of the form [l,r)
											 */

		step = 5;
		last_step = -step;

		if(verbose) cout << "Parsing input ..." << endl;

		/*
		 * Step 2: start parsing
		 */

		while(j<n){

			if(verbose){

				int perc = (100*j)/n;
				if(perc>last_step+(step-1)){

					last_step = perc;
					cout << " " << perc << "% done ..." << endl;

				}

			}

			sparse_vec& B = SA[c];

			auto u = RLBWT.number_of_runs(range);

			if(u==1 or B.exists_non_NIL(range)){

				if(u>1){

					assert(B.find_non_NIL(range) >= l);

					p = B.find_non_NIL(range) - l;

				}

				l++;
				range = RLBWT.LF(range,c);

			}else{

				auto start = (char*)(new ulint(p-1));
				auto len = (char*)(new ulint(l));

				assert(c!=RLBWT.get_terminator());
				auto cc = (char)c;

				out.write(start,sizeof(ulint));
				out.write(len,sizeof(ulint));
				out.write(&cc,1);

				delete start;
				delete len;

				l = 0;
				p = 0;
				range = {0,n};

			}

			auto range_run = RLBWT.locate_run(k);
			B.update_interval(j,k,range_run);

			j++;
			k = RLBWT.LF(k);
			c = RLBWT[k];

		}

	}

private:

	//the run-length encoded BWT
	rle_bwt RLBWT;

	//suffix array samples (two per BWT run)
	//one (sparse) vector of samples per character
	map<char_t,sparse_vec> SA;

};

}

#endif /* INCLUDE_ALGORITHMS_rle_lz77_V1_HPP_ */
