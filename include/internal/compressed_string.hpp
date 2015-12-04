/*
 * compressed_string.hpp
 *
 *  Created on: Nov 30, 2015
 *      Author: nico
 *
 *  Dynamic string supporting rank, select, access, insert.
 *
 *  The string uses a wavelet tree. The encoding depends on the constructor:
 *
 *  - dynamic_string() : gamma coding. maximum code bit-length will be 2log sigma. New characters can always
 *     be inserted.
 *  - dynamic_string(uint64_t sigma) : fixed-length. Max sigma characters are allowed
 *  - dynamic_string(vector<pair<char_type,double> > P) : Huffman-encoding. The characters set is fixed at construction time.
 *
 */

#ifndef INCLUDE_INTERNAL_COMPRESSED_STRING_HPP_
#define INCLUDE_INTERNAL_COMPRESSED_STRING_HPP_

#include <includes.hpp>
#include <alphabet_encoder.hpp>

namespace dyn{

template<class dynamic_bitvector_t>
class compressed_string{

public:

	//we allow any alphabet
	typedef uint64_t char_type;

	/*
	 * Constructor #1
	 *
	 * Alphabet is unknown. Characters are gamma-coded
	 *
	 */
	compressed_string(){}

	/*
	 * Constructor #2
	 *
	 * We know only alphabet size. Each character is assigned log2(sigma) bits.
	 * Characters are assigned codes 0,1,2,... in order of appearance
	 *
	 */
	compressed_string(uint64_t sigma){

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
	compressed_string(vector<pair<char_type,double> >& P){

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

		assert(ae.char_exists(c));
		assert(i<rank(size(),c));

		auto code = ae.encode(c);

		//if this fails, it means that c is not present
		//in the string or that it is not present
		//in the initial dictionary (if any)
		assert(code.size()>0);

		return root.select(i,code, ae);

	}

	/*
	 * number of chars equal to c before position i EXCLUDED
	 */
	uint64_t rank(uint64_t i, char_type c){

		assert(i<=size());

		if(not ae.char_exists(c)) return 0;

		auto code = ae.encode(c);

		//if this fails, it means that c is not present
		//in the string or that it is not present
		//in the initial dictionary (if any)
		assert(code.size()>0);

		return root.rank(i,code, ae);

	}

	bool char_exists(char_type c){

		return ae.char_exists(c);

	}

	void push_back(char_type c){

		insert(size(),c);

	}

	void push_front(char_type c){

		insert(0,c);

	}

	/*
	 * insert a bit set at position i
	 */
	void insert(uint64_t i, char_type c){

		//get code of c
		//if code does not yet exist, create it
		auto code = ae.encode(c);

		root.insert(i,code,c);

		n++;

	}

	uint64_t bit_size() {

		//TODO
		assert(false);
		return 0;

	}

	ulint alphabet_size(){

		return ae.size();

	}

private:

	class node{

	public:

		//root constructor
		node(){}

		//parent constructor
		node(node* parent_){

			this->parent_ = parent_;

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

			if(b){

				return child1_->at( bv.rank1(i) );

			}

			return child0_->at( bv.rank0(i) );

		}

		/*
		 * insert code B[j,...,B.size()-1] at position i. This code is associated
		 * with character c
		 */
		void insert(ulint i, vector<bool>& B, char_type c, ulint j=0){

			if(j==B.size()){

				//this node must be a leaf
				assert(bv.size()==0);

				if(is_leaf()){

					//if it's already marked as leaf, check
					//that the label is correct
					assert(c==label());

				}else{

					//else, mark node as leaf
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

				assert(j == B.size()-1 or bv.rank1(i) <= child1_->bv.size());

				child1_->insert( bv.rank1(i), B, c, j+1 );

			}else{

				if(not has_child0()){
					child0_ = new node(this);
				}

				assert(j == B.size()-1 or  bv.rank0(i) <= child0_->bv.size());

				child0_->insert( bv.rank0(i), B, c, j+1 );

			}

		}

		ulint rank(ulint i, vector<bool>& B, alphabet_encoder& ae, ulint j=0){

			assert(j <= B.size());

			if(j==B.size()) return i;

			assert(i <= bv.size());

			if(B[j]){

				assert(bv.rank1(bv.size())>0);
				assert(has_child1());
				return child1_->rank( bv.rank1(i), B, ae, j+1 );

			}

			assert(bv.rank0(bv.size())>0);
			assert(has_child0());
			return child0_->rank( bv.rank0(i), B, ae, j+1 );

		}


		ulint select(ulint i, vector<bool>& B, alphabet_encoder& ae){

			assert( ae.code_exists(B) );

			//top-down: find leaf associated with B
			node* L = get_leaf(B);

			//bottom-up: from leaf to root
			return (L->parent_)->select(i,B,B.size()-1);

		}

		ulint select(ulint i, vector<bool>& B, ulint j){

			if(j==0){

				assert(is_root());

				return bv.select(i,B[0]);

			}

			return parent_->select( bv.select(i,B[j]), B, j-1);

		}

		//get leaf associated to code B
		node* get_leaf(vector<bool>& B, ulint j=0){

			assert(j<=B.size());

			if(j==B.size()) return this;

			if(B[j]){

				return child1_->get_leaf(B,j+1);

			}

			return child0_->get_leaf(B,j+1);

		}

		bool is_root(){ return parent_ == NULL; }
		bool is_leaf(){ return is_leaf_; }
		bool has_child0(){ return child0_ != NULL; }
		bool has_child1(){ return child1_ != NULL; }

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
		node* parent_ = NULL;		//parent (NULL if root)

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


#endif /* INCLUDE_INTERNAL_COMPRESSED_STRING_HPP_ */
