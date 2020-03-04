// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

//============================================================================
// Name        : h0_lz77.hpp
// Author      : Nicola Prezza
// Version     : 1.0

/*
 * Builds online and in compressed space the LZ77 parse of an input stream.
 * Uses a dynamic compressed BWT as main structure. Zero-order compressed space, n log n time.
 *
 * From the paper: Alberto Policriti and Nicola Prezza, "Fast Online Lempel-Ziv Factorization in Compressed Space"
 *
 */
//============================================================================


#ifndef H0_LZ77_PARSER_H_
#define H0_LZ77_PARSER_H_

#include <dynamic.hpp>

namespace dyn {

template <	class dyn_fmi	//dynamic Fm index
		>
class h0_lz77 {

public:

	/*
	 * Constructor #1: chars are gamma-coded
	 */
	h0_lz77(){}

	/*
	 * Constructor #2
	 *
	 * We know only alphabet size. Each char is assigned log2(sigma) bits.
	 * Characters are assigned codes 0,1,2,... in order of appearance
	 *
	 */
	h0_lz77(uint64_t sigma, ulint sample_rate = DEFAULT_SA_RATE){

		assert(sigma>0);
		fmi = dyn_fmi(sigma, DEFAULT_SA_RATE);

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
	 * Here chars are Huffman encoded.
	 *
	 */
	h0_lz77(istream& in, ulint sample_rate = DEFAULT_SA_RATE){

		auto freqs = get_frequencies(in);
		fmi = dyn_fmi(freqs, sample_rate);

	}

	/*
	 * number of bits required to write down x>0
	 */
	uint64_t bit_size(uint64_t x){

		assert(x>0);

		return 64 - __builtin_clzll(x);

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
	 * input: an input stream and an output stream
	 * the algorithms scans the input (just 1 scan) and
	 * saves to the output stream (could be a file) a series
	 * of triples <pos,len,c> of type <ulint,ulint,uchar>. Types
	 * are converted to char* before streaming them to out
	 * (i.e. ulint to 8 bytes and uchar to 1 byte). len is the length
	 * of the copied string (i.e. excluded skipped characters in the end)
	 *
	 * after the end of a phrase, skip 'skip'>0 characters, included trailing character (LZ77
	 * sparsification, experimental)
	 *
	 * to get also the last factor, input stream should
	 * terminate with a character that does not appear elsewhere
	 * in the stream
	 *
	 */
	void parse(istream& in, ostream& out, ulint skip = 1, bool verbose = false){

		//size of the output if this is compressed using gamma/delta encoding
		uint64_t gamma_bits = 0;
		uint64_t delta_bits = 0;

		assert(skip>0);

		long int step = 1000000;	//print status every step characters
		long int last_step = 0;

		assert(fmi.size()==1);	//only terminator

		pair<ulint, ulint> range = fmi.get_full_interval();	//BWT range of current phrase

		ulint len = 0;	//length of current LZ phrase
		ulint i = 0;	//position of terminator character in bwt
		ulint p = 0;	//phrase occurrence

		ulint z = 0; 	//number of LZ77 phrases

		if(verbose) cout << "Parsing ..." << endl;

		char cc;
		while(in.get(cc)){

			//cout << cc;

			if(verbose){

				if(fmi.text_length()>last_step+(step-1)){

					last_step = fmi.text_length();
					cout << " " << fmi.text_length() << " characters processed ..." << endl;

				}

			}

			uchar c(cc);

			auto new_range = fmi.LF(range,c);

			if(new_range.first >= new_range.second){

				//cout << ":";

				//empty range: new factor

				ulint occ;

				if(len>0){

					occ = i == range.first ? range.second-1 : range.first;
					p = fmi.locate(occ) - len;

				}

				fmi.extend(c);

				auto start = (char*)(new ulint(p));
				auto l = (char*)(new ulint(len));

				out.write(start,sizeof(ulint));
				out.write(l,sizeof(ulint));
				out.write(&cc,1);

				gamma_bits += gamma(uint64_t(start));
				gamma_bits += gamma(uint64_t(l));
				gamma_bits += gamma(uint64_t(uint8_t(cc)));

				delta_bits += delta(uint64_t(start));
				delta_bits += delta(uint64_t(l));
				delta_bits += delta(uint64_t(uint8_t(cc)));


				delete start;
				delete l;

				z++;
				len = 0;
				p = 0;

				//skip characters

				ulint k = 0;

				while(k < skip-1 && in.get(cc)){

					//cout << cc;

					fmi.extend(uchar(cc));
					k++;

				}

				//cout << "|";

				range = fmi.get_full_interval();

			}else{

				len++;			//increase current phrase length
				fmi.extend(c);	//insert character c in the BWT
				i = fmi.get_terminator_position();				//get new terminator position
				range = {new_range.first, new_range.second+1};	//new suffix falls inside current range: extend

			}


		}

		if(verbose){

			cout << "\nNumber of LZ77 phrases: " << z << endl;
			cout << "gamma complexity of the output: " << (gamma_bits/8)+1 << " Bytes, " << double(gamma_bits)/double(fmi.text_length()) << " bit/symbol" << endl;
			cout << "delta complexity of the output: " << (delta_bits/8)+1 << " Bytes, " << double(delta_bits)/double(fmi.text_length()) << " bit/symbol" << endl;


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

		return sizeof(h0_lz77<dyn_fmi>)*8 + fmi.bit_size();

	}

	static const ulint DEFAULT_SA_RATE = 256;

private:

	//the dynamic compressed BWT
	dyn_fmi fmi;

};

} /* namespace data_structures */


#endif /* H0_LZ77_PARSER_H_ */
