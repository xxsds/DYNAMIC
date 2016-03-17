/*
 * fm_index.hpp
 *
 *  Created on: Jan 15, 2016
 *      Author: nico
 *
 *  Dynamic FM-index. Supports LF mapping, backward search, locate, left-extend text
 *
 *  Note that positions are enumerated from the end, where BWT terminator has
 *  position 0. e.g. in T = "abcd#", T[0] = # (where # is the BWT terminator)
 *  Note: alphabet character 2^64-1 is reserved for the BWT terminator
 *
 *
 */

#ifndef INCLUDE_INTERNAL_FM_INDEX_HPP_
#define INCLUDE_INTERNAL_FM_INDEX_HPP_

#include <includes.hpp>

template <	class dyn_bwt,	//dynamic BWT
			class dyn_bv,	//dynamic bitvector
			class dyn_vec	//dynamic vector
		>
class fm_index : public dyn_bwt{

public:

	//we allow any alphabet
	using char_type = ulint;

	/*
	 * Constructor #1
	 *
	 * Alphabet is unknown. Characters are gamma-coded.
	 * BWT is initialized with only terminator character (size=1)
	 *
	 */
	fm_index(){

		marked.insert(0,true);
		SA.insert(0,0);
		this->sample_rate = DEFAULT_SA_RATE;

	}

	/*
	 * Constructor #2
	 *
	 * We know only alphabet size. Each character is assigned log2(sigma) bits.
	 * Characters are assigned codes 0,1,2,... in order of appearance
	 * BWT is initialized with only terminator character (size=1)
	 *
	 */
	fm_index(uint64_t sigma, ulint sample_rate = DEFAULT_SA_RATE) : dyn_bwt(sigma){

		marked.insert(0,true);
		SA.insert(0,0);
		this->sample_rate = sample_rate;

	}

	/*
	 * Constructor #3
	 *
	 * We know character probabilities. Input: pairs <character, probability>
	 *
	 * Here the alphabet is Huffman encoded.
	 * BWT is initialized with only terminator character (size=1)
	 *
	 */
	fm_index(vector<pair<char_type,double> >& P, ulint sample_rate = DEFAULT_SA_RATE) : dyn_bwt(P){

		marked.insert(0,true);
		SA.insert(0,0);
		this->sample_rate = sample_rate;

	}


	/*
	 * input: position on F column of the BWT
	 * output: corresponding position on text. Note that
	 * text positions are enumerated from the end, with
	 * the BWT terminator (last character) having position 0.
	 */
	ulint locate(ulint i){

		return locate(i,0);

	}

	/*
	 * input: range [l,r) (right-excluded) of positions on F column of the BWT
	 * output: vector of corresponding positions on text. Note that
	 * text positions are enumerated from the end, with
	 * the BWT terminator (last character) having position 0.
	 */
	vector<ulint> locate(pair<ulint,ulint> range){

		auto res = vector<ulint>();

		for(ulint i=range.first;i<range.second;++i) res.push_back(locate(i));

		return res;

	}

	/*
	 * input: pattern P
	 * output: occurrences of P in the text
	 */
	vector<ulint> locate(vector<char_type> P){

		return locate(dyn_bwt::count(P));

	}

	/*
	 * build FM index of cW from FM index of W
	 */
	void extend(char_type c){

		dyn_bwt::extend(c);	//extend BWT

		/*
		 * position of new suffix in the BWT
		 * matrix (row number)
		 */
		auto tp = this->get_terminator_position();

		if(this->text_length() % sample_rate == 0){

			marked.insert(tp,true);					//mark position with 1
			SA.insert(marked.rank1(tp),this->text_length());	//insert SA sample

		}else{

			marked.insert(tp,false);				//mark position with 0

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

		ulint size = sizeof(fm_index<dyn_bwt,dyn_bv,dyn_vec>)*8;

		size += dyn_bwt::bit_size();
		size += marked.bit_size();
		size += SA.bit_size();

		return size;

	}

	ulint serialize(ostream &out){

		ulint w_bytes=0;

		w_bytes += dyn_bwt::serialize(out);

		out.write((char*)&sample_rate,sizeof(sample_rate));
		w_bytes += sizeof(sample_rate);

		w_bytes += marked.serialize(out);
		w_bytes += SA.serialize(out);

		return w_bytes;

	}

	void load(istream &in){

		dyn_bwt::load(in);

		in.read((char*)&sample_rate,sizeof(sample_rate));

		marked.load(in);
		SA.load(in);

	}

private:

	/*
	 * locate and add j
	 */
	ulint locate(ulint i, ulint j){

		return 	marked[i] ?
				SA[marked.rank1(i)] + j :
				locate( this->FL(i), j+1 );

	}

	dyn_bv marked;	//is position i marked with a SA sample?
	dyn_vec SA;		//suffix array sampling

	ulint sample_rate;	//one SA sample out of sample_rate positions

	static const ulint DEFAULT_SA_RATE = 256;

};



#endif /* INCLUDE_INTERNAL_FM_INDEX_HPP_ */
