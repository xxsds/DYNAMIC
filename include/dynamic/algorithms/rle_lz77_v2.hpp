// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

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
 *  Space is O(R log n + z log n) bits. The constants hidden in the
 *  big-O notation are much smaller than in rle_lz77_v1.hpp:
 *  in this algorithm, space usage is around (2R + 3z) log n bits
 *
 *  Type of input text here is uchar
 *
 *
 */

#ifndef INCLUDE_ALGORITHMS_RLE_LZ77_V2_HPP_
#define INCLUDE_ALGORITHMS_RLE_LZ77_V2_HPP_

#include "dynamic/dynamic.hpp"
#include <unordered_map>

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
	 * of triples <pos,len,c> of type <ulint,ulint,uchar>. Types
	 * are converted to char* before streaming them to out
	 * (i.e. ulint to 8 bytes and uchar to 1 byte)
	 *
	 * to get also the last factor, input stream should
	 * terminate with a character that does not appear elsewhere
	 * in the stream
	 *
	 */
	void parse(istream& in, ostream& out, bool verbose = false){

		long int step = 1000000;	//print status every step characters
		long int last_step = 0;

		ulint pos = 0;			/* text characters processed */

		ulint l = 0;			/* Length of current LZ phrase prefix */

		ulint z = 0;			/* LZ phrase counter */

		pair<ulint, ulint> range = {0,1};	/* range of current LZ phrase prefix
											 * full interval is <0,n> : intervals
											 * are of the form [l,r). At the beginning,
											 * only BWT terminator is in the BWT
											 */

		/* STEP 1: parse input and store LZ-factors start positions in BWT-coordinate space */

		if(verbose) cout << "Parsing input and building RLBWT ..." << endl;

		SA.insert_NIL(0);	/* at the beginning, RLBWT contains only the terminator */

		char cc;
		while(in.get(cc)){

			auto c = uchar(cc);

			if(verbose){

				if(pos>last_step+(step-1)){

					last_step = pos;
					cout << " " << pos << " characters processed ..." << endl;

				}

			}

			/* copy range */
			pair<ulint, ulint> range_temp(range);

			/* extend search */
			range = RLBWT.LF(range,c);

			if(range.second <= range.first){

				/* empty range: end of a LZ factor */

				factors_char.push_back( c );
				factors_len.push_back( l );

				if(l>0){

					/*j = start position of LZ factor, but in BWT coordinate-space */
					auto j = range_temp.first;

					auto SA_j = SA[j];

					if(SA_j == SA.get_NIL()){

						/* no sample here. Insert a new sample */

						/* SA[j] points to the last bit set (appended with
						 * the next instruction) in rep */
						auto r = rep.rank1(rep.size());
						SA[j] = r;

						/* append bit set in rep */
						rep.push_back(true);

						/* append pointer to LZ factors in ptr*/
						ptr.push_back(z);

					}else{

						/* this position is already marked (i.e. used by a LZ factor). */

						/* SA[j]=t refers to the t-th bit set in rep*/
						auto k = rep.select1(SA_j);

						/* insert a 0 in rep */
						rep.insert0(k);

						/* insert current LZ phrase index in ptr */
						ptr.insert(k,z);

					}

				}

				/* reset range. Add 1 to right border because we still have to insert c */
				range = {0,RLBWT.size()+1};

				/* increment factor index */
				z++;

				/* reset phrase length */
				l = 0;

			}else{

				l++;
				range.second++; //new suffix falls inside range

			}

			RLBWT.extend( c );
			SA.insert_NIL(RLBWT.get_terminator_position());

			pos++;

		}

		/* STEP 2 : convert LZ factors start coordinates from BWT-coordinate space to text-coordinate space*/

		if(verbose) cout << "Converting BWT coordinates to text coordinates ..." << endl;

		ulint n = RLBWT.size()-1; /* text length */

		ulint width = 64 - __builtin_popcountll( n );	/* number of bits to store a text address */

		factors_start = packed_vector(z,width);

		ulint j = 0; /* position on text */
		ulint k = RLBWT.LF(0); /* position on F column of RLBWT corresponding to text position j*/

		step = 5;
		last_step = -step;

		while (j<n){

			if(verbose){

				int perc = (100*j)/n;
				if(perc>last_step+(step-1)){

					last_step = perc;
					cout << " " << perc << "% done ..." << endl;

				}

			}

			auto r = SA[k];

			if(r != SA.get_NIL()){

				l = r == 0 ? 0 : rep.select1(r-1)+1;
				r = rep.select1(r);

				for(ulint i = l; i<=r;++i){

					auto factor = ptr[i];
					auto len = factors_len[factor];

					assert(factor<factors_start.size());

					factors_start[factor] = len == 0 ? 0 : j - (len-1);

				}

			}

			j++;
			k = RLBWT.LF(k);

		}

		assert(factors_start.size() == z);
		assert(factors_len.size() == z);
		assert(factors_char.size() == z);

		uint64_t cumulative=1; //sum of phrase before current position (we start with terminator)
		uint64_t gamma_bits = 0;
		uint64_t delta_bits = 0;

		vector<uint64_t> off;
		uint64_t sum_log = 0;

		for(ulint j=0;j<z;++j) {

			auto start = (char*)(new ulint(factors_start[j]));
			auto len = (char*)(new ulint(factors_len[j]));

			assert(factors_char[j]!=RLBWT.get_terminator());

			auto cc = (char)factors_char[j];

			out.write(start,sizeof(ulint));
			out.write(len,sizeof(ulint));
			out.write(&cc,1);

			if(factors_len[j] > 0 and cumulative < uint64_t(factors_start[j])+1){

				cout << "err: " << cumulative << "/" << factors_start[j] << endl;
				exit(0);

			}

			uint64_t backward_pos = uint64_t(factors_len[j]) == 0 ? 1 : cumulative - uint64_t(factors_start[j]);

			off.push_back(backward_pos);

			sum_log += bit_size(backward_pos);

			gamma_bits += gamma(uint64_t(backward_pos+1));
			gamma_bits += gamma(uint64_t(uint64_t(factors_len[j])+1));
			gamma_bits += gamma(uint64_t(uint8_t(cc)));

			delta_bits += delta(uint64_t(backward_pos+1));
			delta_bits += delta(uint64_t(uint64_t(factors_len[j])+1));
			delta_bits += delta(uint64_t(uint8_t(cc)));

			cumulative += (uint64_t(factors_len[j]) + 1);

			delete start;
			delete len;

		}


		if(verbose){

			cout << "Done. Number of phrases: " << z << endl;
			cout << "Entropy of the offsets: " << entropy(off) << endl;
			cout << "Sum of logs of the offsets: " << sum_log << endl;
			cout << "gamma complexity of the output: " << (gamma_bits/8)+1 << " Bytes, " << double(gamma_bits)/double(RLBWT.text_length()) << " bit/symbol" << endl;
			cout << "delta complexity of the output: " << (delta_bits/8)+1 << " Bytes, " << double(delta_bits)/double(RLBWT.text_length()) << " bit/symbol" << endl;

		}

	}

	/*
	 * Total number of bits allocated in RAM for this structure
	 *
	 * WARNING: this measure is good only for relatively small alphabets (e.g. ASCII)
	 * as we use STL containers such as set and map which do not give direct info on
	 * the total memory allocated. The sizes of these containers are proportional
	 * to the alphabet size (but the constants involved are high since internally
	 * they can use heavy structures as RBT)
	 */
	ulint bit_size(){

		ulint size = sizeof(rle_lz77_v2)*8;

		size += RLBWT.bit_size();
		size += SA.bit_size();
		size += rep.bit_size();
		size += ptr.bit_size();

		size += factors_start.bit_size();
		size += factors_len.bit_size();
		size += factors_char.bit_size();

		return size;

	}

private:

	/*
	 * number of bits required to write down x>0
	 */
	uint64_t bit_size(uint64_t x){

		return x==0 ? 1 : 64 - __builtin_clzll(x);

	}

	/*
	 * compute the bit-size of the gamma encoding of x
	 */
	uint64_t gamma(uint64_t x){

		return 2*bit_size(x) - 1;

	}

	/*
	 * compute the bit-size of the delta encoding of x
	 */
	uint64_t delta(uint64_t x){

		auto bits = bit_size(x);//bits needed to encode x

		return gamma(bits) + bits -1;

	}

	/*
	 * entropy of a vector
	 */
	double entropy(vector<uint64_t> & V){

		unordered_map<uint64_t, double> freq;
		double n = V.size();

		for(auto x : V)	freq[x]=0;
		for(auto x : V)	freq[x]++;

		double H = 0;

		for(auto p : freq){

			auto f = p.second/n;

			H -= f*log2(f);

		}

		return H;

	}


	//the run-length encoded BWT
	rle_bwt RLBWT;

	//suffix array samples. These are pointers to the bits set in rep
	sparse_vec SA;

	/*
	 * a BWT position can be shared as source by more than 1 LZ factor.
	 * If SA[i] != NIL, then ptr[ rep.select1( SA.rank(i) ), ..., rep.select1( SA.rank(i) +1)-1 ]
	 * are pointers to LZ factors that share the BWT position i as source.
	 */
	suc_bv rep;	/* 00001 */
	packed_spsi ptr;

	/* the LZ factors. These 3 vector stores z integers each:
	 *
	 * start, length and trailing character of each LZ factor.
	 * Length is the length of a factor minus 1 (we do not
	 * count the trailing character).
	 */
	packed_vector factors_start;
	packed_vector factors_len;
	packed_vector factors_char;

};

}

#endif /* INCLUDE_ALGORITHMS_RLE_LZ77_V2_HPP_ */
