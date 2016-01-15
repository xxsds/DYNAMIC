//============================================================================
// Name        : ho_lz77.hpp
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
	 * Constructor #1: run-heads are gamma-coded
	 */
	h0_lz77(){}

	/*
	 * Constructor #2
	 *
	 * We know only alphabet size. Each Run-head char is assigned log2(sigma) bits.
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
	 * Here Run-heads are Huffman encoded.
	 *
	 */
	h0_lz77(istream& in, ulint sample_rate = DEFAULT_SA_RATE){

		auto freqs = get_frequencies(in);
		fmi = dyn_fmi(freqs, sample_rate);

	}

	/*
	 * input: an input stream and an output stream
	 * the algorithms scans the input (just 1 scan) and
	 * saves to the output stream (could be a file) a series
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

		long int step = 100000;	//print status every step characters
		long int last_step = 0;

		assert(fmi.size()==1);	//only terminator

		pair<ulint, ulint> range = fmi.get_full_interval();	//BWT range of current phrase

		ulint len = 0;	//length of current LZ phrase
		ulint i = 0;	//position of terminator character in bwt
		ulint p = 0;	//phrase occurrence

		if(verbose) cout << "Parsing ..." << endl;

		char cc;
		while(in.get(cc)){

			if(verbose){

				if(fmi.text_length()>last_step+(step-1)){

					last_step = fmi.text_length();
					cout << " " << fmi.text_length() << " characters processed ..." << endl;

				}

			}

			uchar c(cc);

			auto new_range = fmi.LF(range,c);

			if(new_range.first >= new_range.second){

				//empty range: new factor

				ulint occ;

				if(len>0){

					occ = i == range.first ? range.second-1 : range.first;
					p = fmi.locate(occ) - len;

				}

				fmi.extend(c);
				range = fmi.get_full_interval();

				//cout << p << " " << len << " " << cc << endl;

				auto start = (char*)(new ulint(p));
				auto l = (char*)(new ulint(len));

				out.write(start,sizeof(ulint));
				out.write(l,sizeof(ulint));
				out.write(&cc,1);

				delete start;
				delete l;

				len = 0;
				p = 0;

			}else{

				len++;			//increase current phrase length
				fmi.extend(c);	//insert character c in the BWT
				i = fmi.get_terminator_position();				//get new terminator position
				range = {new_range.first, new_range.second+1};	//new suffix falls inside current range: extend

			}


		}

	}


private:

	//the dynamic compressed BWT
	dyn_fmi fmi;

	static const ulint DEFAULT_SA_RATE = 256;

};

} /* namespace data_structures */


#endif /* H0_LZ77_PARSER_H_ */