/*
 * dynamic_string.hpp
 *
 *  Created on: Nov 30, 2015
 *      Author: nico
 *
 *  Dynamic string supporting rank, select, access, insert.
 *
 *  The string uses a wavelet tree. The encoding depends on the constructor with which the
 *  string is built:
 *
 *  - dynamic_string() : gamma coding. maximum code length will be 2log sigma. New characters can always
 *     be inserted.
 *  - dynamic_string(uint64_t sigma) : fixed-length. Max sigma characters are allowed
 *  - dynamic_string(vector<pair<char_type,double> > P) : Huffman-encoding. The characters set is fixed at construction time.
 *
 */

#ifndef INCLUDE_INTERNAL_DYNAMIC_STRING_HPP_
#define INCLUDE_INTERNAL_DYNAMIC_STRING_HPP_

#include <includes.hpp>
#include <alphabet_encoder.hpp>

namespace dyn{

template<class dynamic_bitvector_t>
class dynamic_string{

public:

	/*
	 * Constructor #1
	 *
	 * Alphabet is unknown. Characters are gamma-coded
	 *
	 */
	dynamic_string(){}

	/*
	 * Constructor #2
	 *
	 * We know only alphabet size. Each character is assigned log2(sigma) bits.
	 * Characters are assigned codes 0,1,2,... in order of appearance
	 *
	 */
	dynamic_string(uint64_t sigma){

		assert(sigma>0);
		ae = alphabet_encoder(sigma);

	}

	/*
	 * Constructor #3
	 *
	 * We know character probabilities. Input: pairs <character, probability>
	 *
	 * Here the alphabet is Huffman encoded.
	 *
	 */
	dynamic_string(vector<pair<char_type,double> >& P){

		ae = alphabet_encoder(P);

	}


	/*
	 * number of bits in the bitvector
	 */
	uint64_t size(){

		return n;

	}

	/*
	 * high-level access to the string. Supports assign (operator=) and access
	 */
	char_type operator[](uint64_t i){

		return at(i);

	}

	/*
	 * access
	 */
	char_type at(uint64_t i){

		assert(i<size());
		return root.at(i);

	}

	/*
	 * position of i-th character equal to c. 0 =< i < rank(size(),c)
	 */
	uint64_t select(uint64_t i,char_type c){

		assert(i<rank(size(),c));
		return 0;

	}

	/*
	 * number of chars equal to c before position i EXCLUDED
	 */
	uint64_t rank(uint64_t i, char_type c){

		assert(i<=size());
		return 0;

	}

	/*
	 * insert a bit set at position i
	 */
	void insert(uint64_t i, char_type c){

		//get code of c
		//if code does not yet exist, create it
		auto code = ae.encode(c);

		//for(auto b:code) cout << b;cout<<endl;

		root.insert(i,code,c);

		n++;

	}

	uint64_t bit_size() {

		return 0;

	}


private:

	class node{

	public:

		//root constructor
		node(){}

		//parent constructor
		node(node* parent_){

			this->p_ = parent_;

		}

		~node(){

			if(has_child0()){

				delete child0_;
				child0_=NULL;

			}

			if(has_child1()){

				delete child1_;
				child1_=NULL;

			}

		}

		//turn this node into a leaf
		void make_leaf(char_type c){

			assert(not has_child0()); //musttnot have children
			assert(not has_child1());
			assert(bv.size()==0);

			assert(not is_root()); //cannot make the root leaf

			this->is_leaf_ = true;
			this->l_=c;

		}

		//descend tree and return character at this position
		char_type at(ulint i){

			if(is_leaf()) return label();

			assert(i<bv.size());

			bool b = bv[i];

			assert((b or has_child0()) and (not b or has_child1()));

			if(b) return (child1())->at( bv.rank1(i) );
			return (child0())->at( bv.rank0(i) );

		}

		/*
		 * insert code B[j,...,B.size()-1] at position i. This code is associated
		 * with character c
		 */
		void insert(ulint i, vector<bool>& B, char_type c, ulint j=0){

			if(j==B.size()){

				if(is_leaf()){

					assert(c==label());

				}else{

					make_leaf(c);

				}

				return;

			}

			assert(i<=bv.size());
			assert(not is_leaf());

			bool b = B[j];
			bv.insert(i,b);

			if(b){

				//if child does not exist, create it.
				if(not has_child1()){
					child1_ = new node(this);
				}

				child1_->insert( bv.rank1(i), B, c, j+1 );

			}else{

				if(not has_child0()){
					child0_ = new node(this);
				}

				child0_->insert( bv.rank0(i), B, c, j+1 );

			}

		}

		bool is_root(){ return parent() == NULL; }
		bool is_leaf(){ return is_leaf_; }
		bool has_child0(){ return child0_ != NULL; }
		bool has_child1(){ return child1_ != NULL; }

		node* child0(){ assert(has_child0()); return child0_; }
		node* child1(){ assert(has_child1()); return child1_; }
		node* parent(){ return p_; }

		/*
		 * to have left label, this node must not have a left (0)
		 * child AND must have at least one bit equal to 0 in
		 * its bitvector
		 */
		char_type label(){

			assert(is_leaf());
			return l_;

		}

	private:

		bool is_leaf_ = false;

		node* child0_ = NULL;
		node* child1_ = NULL;
		node* p_ = NULL;		//parent (NULL if root)

		//if is_leaf_, then node is labeled
		char_type l_ = 0;

		dynamic_bitvector_t bv;

	};

	//current length
	ulint n = 0;

	node root;

	alphabet_encoder ae;

};

}


#endif /* INCLUDE_INTERNAL_DYNAMIC_STRING_HPP_ */
