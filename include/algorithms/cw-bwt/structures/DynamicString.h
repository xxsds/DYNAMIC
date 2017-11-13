// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.


/*
 * DynamicString.h
 *
 *  Created on: Jun 19, 2014
 *      Author: nicola
 */

#ifndef DYNAMICSTRING_H_
#define DYNAMICSTRING_H_

#include "includes.hpp"
#include "HuffmanTree.h"
#include "dynamic.hpp"

#include <sstream>

using namespace dyn;

namespace bwtil {

//definition of the bitvector used
//typedef DummyDynamicBitvector bitv;

template <typename bitvector_type>
class DynamicString {

public:

	DynamicString(){n=0;current_size=0;unary_string=true;sigma=0;sigma_0=0;H0=0;};

	ulint rank(symbol x, ulint i){

		if(n==0)
			return 0;

	#ifdef DEBUG
		if(i>current_size){

			cout << "ERROR (DynamicString): trying to compute rank in position outside current string : " << i << ">" << current_size << endl;
			exit(0);

		}
	#endif

		if(unary_string)
			return i;

		vector<bool> code = codes.at(x);//bitvector to be searched in the wavelet tree
		return rank(&code, 0, 0, i);

	}

	ulint size(){return current_size;};//current size

	ulint maxLength(){return n;}

	double entropy(){return H0;}

	ulint bitSize(){

		ulint bits=0;

		for(uint64_t i=0;i<codes.size();i++)
			bits += codes.at(i).size();

		bits += 8* sizeof(this);
		bits += 8*codes.capacity()*sizeof(vector<bool>);
		bits += 8*sizeof(codes);
		bits += 8*number_of_internal_nodes*sizeof(bitvector_type *);
		bits += 8*number_of_internal_nodes*sizeof(bitvector_type);
		bits += 2*8*number_of_internal_nodes*sizeof(uint16_t);

		return  bits;
	}

	DynamicString(vector<ulint> freq){//absolute frequencies of the characters

		n=0;

		for(uint64_t i=0;i<freq.size();i++)
			n+=freq.at(i);

		if(n==0){
			current_size=0;
			unary_string=true;
			sigma=0;
			sigma_0=0;
			H0=0;
			return;
		}

	#ifdef DEBUG
		if(freq.size()>255){
			cout << "ERROR (DynamicString): Maximum size of the alphabet is 255. (input alphabet size is " << freq.size() << ")\n";
			exit(0);
		}

		this->freq = vector<ulint>(freq.size());

		for(ulint i=0;i<freq.size();i++)
			this->freq.at(i) = freq.at(i);

	#endif

		sigma = freq.size();
		sigma_0=0;

		for(uint64_t i=0;i<freq.size();i++)
			if(freq.at(i)>0)
				sigma_0++;

	#ifdef DEBUG
		if(sigma_0==0){
			cout << "ERROR (DynamicString): trying to build dynamic string on a null alphabet\n";
			exit(0);
		}

		current_freqs = vector<ulint>(freq.size());
		for(uint64_t i=0;i<freq.size();i++)
			current_freqs.at(i)=0;
	#endif

		current_size = 0;

		if(sigma_0==1){

			for(uint64_t i=0;i<freq.size();i++)//search the unique char with freq>0
				if(freq.at(i)>0)
					s=i;

			unary_string = true;
			H0 = (uint64_t)log2(n);
			return;

		}

		//Alphabet size is > 1

		unary_string = false;

		HuffmanTree<> ht = HuffmanTree<>(freq);
		codes = ht.getCodes();

		H0 = ht.entropy();

		number_of_internal_nodes = sigma_0-1;

		wavelet_tree.resize(number_of_internal_nodes);

		child0 = vector<uint16_t>(number_of_internal_nodes);
		child1 = vector<uint16_t>(number_of_internal_nodes);

		vector<symbol> alphabet;
		for(symbol i=0;i<freq.size();i++)
			if(freq.at(i)>0)
				alphabet.push_back(i);

		uint64_t next_free_node = 1;
		buildTree(freq,alphabet,0,0,&next_free_node);

	}

	symbol at(ulint i){

		if(n==0)
			return 0;

	#ifdef DEBUG
		if(i>=current_size){

			cout << "ERROR (DynamicString): trying to at position outside current string : " << i << ">=" << current_size << endl;
			exit(0);

		}
	#endif

		if(unary_string){

			return s;

		}

		return at(0,i);

	}

	void insert(symbol x, ulint i){

		if(n==0)
			return;

	#ifdef DEBUG
		if(i>current_size){

			cout << "ERROR (DynamicString): trying to insert in position outside current string : " << i << ">" << current_size << endl;
			exit(0);

		}

		if(current_freqs.at(x)>=freq.at(x)){

			cout << "ERROR (DynamicString): too many symbols " << (uint64_t)x << " inserted!" << endl;
			exit(0);

		}

		current_freqs.at(x) += 1;

	#endif


		if(not unary_string){

			vector<bool> code = codes.at(x);//bitvector to be inserted in the wavelet tree
			insert(&code,0,0,i);

		}

		current_size++;

	}

	string toString(){

		stringstream ss;

		for(ulint i=0;i<size();i++)
			ss << (uint64_t)at(i);

		return ss.str();

	}

	ulint numberOfBits(){//sum of the lengths of the bitvectors

		if(unary_string)
			return n;

		ulint tot=0;

		for(uint64_t i=0;i<number_of_internal_nodes;i++)
			//tot += wavelet_tree[i].maxSize();
			tot += wavelet_tree[i].bit_size();

		return tot;

	}


	/*ulint  sumOfHeights(){//sum of the heights of all bitvectors' B-trees (each multiplied by the length of the bitvector)

		if(unary_string)
			return n;

		ulint tot=0;

		for(uint64_t i=0;i<number_of_internal_nodes;i++)
			//tot += wavelet_tree[i].maxSize()*wavelet_tree[i].height();
			tot += wavelet_tree[i].bit_size() * (wavelet_tree[i].info().height+1);//sum 1 because in bitvector heights start from 0

		return tot;

	}*/

private:

	void buildTree(vector<ulint> freq,vector<symbol> alphabet,uint64_t pos,uint64_t this_node, uint64_t * next_free_node){

		vector<symbol> alphabet0;
		vector<symbol> alphabet1;

		ulint size = 0;//size of the current bitvector

		for(uint64_t i=0;i<alphabet.size();i++){

			size += freq.at(alphabet.at(i));

		}

		wavelet_tree[this_node] = bitvector_type();

		for(uint64_t i=0;i<alphabet.size();i++){

			if(codes.at(alphabet.at(i)).at(pos)==0){//left (bit 0)

				if(codes.at(alphabet.at(i)).size()-1==pos){//leaf on left: save character

					child0[this_node] = sigma+alphabet.at(i);

				}else{

					if(alphabet0.size()==0){//if this is the first symbol seen with bit 0, allocate new tree node
						child0[this_node] = *next_free_node;
						*next_free_node += 1;
					}

					alphabet0.push_back(alphabet.at(i));

				}

			}else{//right (bit 1)

				if(codes.at(alphabet.at(i)).size()-1==pos){//leaf on right: save character

					child1[this_node] = sigma+alphabet.at(i);

				}else{

					if(alphabet1.size()==0){//if this is the first symbol seen with bit 1, allocate new tree node
						child1[this_node] = *next_free_node;
						*next_free_node += 1;
					}

					alphabet1.push_back(alphabet.at(i));

				}

			}

		}

		if(alphabet0.size()>0)
			buildTree(freq,alphabet0,pos+1,child0[this_node],next_free_node);

		if(alphabet1.size()>0)
			buildTree(freq,alphabet1,pos+1,child1[this_node],next_free_node);


	}

	inline void insert(vector<bool> * code, uint64_t node, uint64_t pos, ulint i){

		bool bit = code->at(pos);

		//cout << "insert of " << bit << " in position " << i << "of bitv of size " << wavelet_tree[node].info().capacity << endl;

		wavelet_tree[node].insert( i, bit );

		if(pos+1<code->size()){

			uint64_t next_node = (bit==0?child0[node]:child1[node]);//find next node

			//ulint next_i = wavelet_tree[node].rank(bit,i);
			ulint next_i = wavelet_tree[node].rank(i,bit);

			insert(code, next_node, pos+1, next_i);

		}

	}

	inline symbol at(uint64_t node, ulint i){

			bool bit = wavelet_tree[node].at(i);

			uint64_t next_node = (bit==0?child0[node]:child1[node]);

			if(next_node>=sigma)//next node is leaf:return symbol
				return next_node-sigma;

			//else: next_node is a valid address in wavelet_tree

			ulint next_i = wavelet_tree[node].rank(i,bit);

			return at(next_node, next_i);

		}

	inline ulint rank(vector<bool> * code, uint64_t node, uint64_t pos, ulint i){

		bool bit = code->at(pos);
		//ulint bit_rank = wavelet_tree[node].rank(bit,i);
		ulint bit_rank = wavelet_tree[node].rank(i,bit);

		if(pos+1==code->size())
			return bit_rank;

		uint64_t next_node = (bit==0?child0[node]:child1[node]);//find next node

		return rank(code, next_node, pos+1, bit_rank);

	}

#ifdef DEBUG

	vector<ulint> freq;//debug only: absolute frequencies of the symbols
	vector<ulint> current_freqs;//debug only: number of symbols inserted

#endif

	symbol number_of_internal_nodes=0;
	symbol sigma;//alphabet size
	symbol sigma_0;//number of characters with frequency > 0
	symbol s=0;//unique symbol of the string if unary alphabet

	vector<uint16_t> child0;//for each node, pointer to left child in wavelet_tree (if any; otherwise sigma+s, where s is the symbol associated to the leaf)
	vector<uint16_t> child1;//for each node, pointer to right child in wavelet_tree (if any; otherwise sigma+s, where s is the symbol associated to the leaf)

	vector<bitvector_type>  wavelet_tree;//internal nodes of the wavelet tree (number_of_internal_nodes in total)
	//tree topology

	vector<vector<bool> > codes;//Huffman codes

	ulint current_size;

	double H0;//0-th order entropy reached by the Huffman compressor

	ulint n;//max size of the string

	bool unary_string;//alphabet has size 1

};

typedef DynamicString<suc_bv> dynamic_string_t;

} /* namespace bwtil */
#endif /* DYNAMICSTRING_H_ */
