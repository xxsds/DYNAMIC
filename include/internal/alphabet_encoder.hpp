/*
 * alphabet_encoder.hpp
 *
 *  Created on: Nov 2, 2015
 *      Author: nico
 *
 *  Encodes and decodes a (possibly dynamic) alphabet.
 *
 *  User can choose (at constructor time) between 3 types of encodings:
 *
 *  - fixed-size: number of bits of each char is fixed. Dynamic (but alphabet size is limited)
 *  - gamma encoding: alphabet is completely unknown at construction time. Dynamic (alphabet size < 2^64)
 *  - Huffman encoding: character probabilities are known at construction time. Static.
 *
 */

#ifndef INCLUDE_INTERNAL_ALPHABET_ENCODER_HPP_
#define INCLUDE_INTERNAL_ALPHABET_ENCODER_HPP_

#include "includes.hpp"
#include <map>

namespace dyn {

//we allow any alphabet
typedef uint64_t char_type;

class alphabet_encoder{

public:

	/*
	 * Constructor #1
	 *
	 * Alphabet is unknown. Characters are gamma-coded
	 *
	 */
	alphabet_encoder(){

		sigma = 0;
		enc_type = gamma;

	}

	/*
	 * Constructor #2
	 *
	 * We know only alphabet size. Each character is assigned log2(sigma) bits.
	 * Characters are assigned codes 0,1,2,... in order of appearance
	 *
	 */
	alphabet_encoder(uint64_t sigma){

		assert(sigma>0);

		this->log_sigma = 64-__builtin_clzll(sigma-1);
		this->sigma = 0;
		enc_type = fixed;

	}

	/*
	 * Constructor #3
	 *
	 * We know character probabilities. Input: pairs <character, probability>
	 *
	 * Here the alphabet is Huffman encoded.
	 *
	 */
	alphabet_encoder(vector<pair<char_type,double> > P){

		sigma = P.size();
		enc_type = huffman;

		auto comp = [](node x, node y){ return x.second < y.second; };
		multiset<node,decltype(comp)> s(comp);

		//insert leaves
		for(auto it = P.begin();it!=P.end();++it)
			s.insert({{NULL,&it->first},it->second});

		//Huffman algorithm
		while(s.size()>1){

			//extract, copy and erase the 2 smallest elements
			auto min1 = new node(*s.begin());
			s.erase(s.begin());

			auto min2 = new node(*s.begin());
			s.erase(s.begin());

			double new_prob = min1->second + min2->second;
			pair<void*,void*> children = {min1,min2};

			s.insert({ children, new_prob });

		}

		node root = *s.begin();
		extract_codes(&root,{});

	}

	/*
	 * input: a character
	 * output: its code
	 *
	 * with constructors #1 and #2, alphabet is not pre-determined: new codes are created for
	 * new coming characters
	 *
	 */
	vector<bool> encode(char_type c){

		auto code = encode_[c];

		//if c does not have a code, then encoding must not be Huffman (which is static)
		assert(code.size() > 0 or enc_type != huffman);

		/*
		 * if character is not present in dictionary, then
		 * allocate new code
		 */
		if(code.size() == 0){

			if(enc_type==gamma){

				encode_[c] = get_new_gamma();

			}else if(enc_type==fixed){

				encode_[c] = get_new_fixed();

			}

			//0 is reserved
			decode_[ encode_[c] ] = c+1;

		}

		return encode_[c];

	}

	char_type decode(vector<bool> code){

		//code must be present in dictionary!
		assert(decode_[code]!=0);

		return decode_[code]-1;

	}

	/*
	 * alphabet size
	 */
	uint64_t size(){
		return sigma;
	}

private:

	//recursive tree definition
	//left/right children are stored in the 2 void pointers
	//if the left void pointer is == NULL, then right pointer
	//is a char_type (i.e. leaf)
	typedef pair<pair<void*,void*>,double> node;

	void extract_codes(node* n, vector<bool> C){

		if(is_leaf(n)){

			assert(C.size()>0);

			encode_[label(n)] = C;
			decode_[C] = label(n)+1;

		}else{

			vector<bool> l = C;
			vector<bool> r = C;

			l.push_back(false);
			r.push_back(true);

			extract_codes(left(n),l);
			extract_codes(right(n),r);

		}

	}

	bool is_leaf(node* n){

		return n->first.first==NULL;

	}

	node* left(node* n){

		assert(not is_leaf(n));
		return (node*)n->first.first;

	}

	node* right(node* n){

		assert(not is_leaf(n));
		return (node*)n->first.second;

	}

	char_type label(node *n){

		assert(is_leaf(n));

		return *((char_type*)n->first.second);

	}

	/*
	 * increment sigma and return gamma code of sigma
	 */
	vector<bool> get_new_gamma(){

		vector<bool> C;

		sigma++;

		//bit length of sigma
		uint8_t len = 64-__builtin_clzll(sigma);

		for(uint8_t i =0;i<len-1;++i) C.push_back(false);

		for(uint8_t i =0;i<len;++i) C.push_back( (sigma >> (len-i-1)) & uint64_t(1) );

		return C;

	}

	/*
	 * increment sigma and return fixed-length code of sigma
	 */
	vector<bool> get_new_fixed(){

		assert(sigma < uint64_t(1)<<log_sigma);

		vector<bool> C;

		if(sigma==0){

			for(uint8_t i =0;i<log_sigma;++i) C.push_back( false );

		}else{

			for(uint8_t i =0;i<log_sigma;++i) C.push_back( (sigma >> (log_sigma-i-1)) & uint64_t(1) );

		}

		sigma++;

		return C;

	}

	enum type {huffman, gamma, fixed};

	map<char_type,vector<bool> > encode_;

	//char_type value 0 is reserved
	map<vector<bool>, char_type> decode_;

	uint64_t sigma;

	uint64_t log_sigma = 0;//used only with fixed size

	type enc_type;

};

}

#endif /* INCLUDE_INTERNAL_ALPHABET_ENCODER_HPP_ */
