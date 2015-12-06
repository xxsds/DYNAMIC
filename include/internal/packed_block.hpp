/*
 * packed_block.hpp
 *
 *  Created on: Oct 21, 2015
 *      Author: nico
 */

#ifndef INTERNAL_PACKED_BLOCK_HPP_
#define INTERNAL_PACKED_BLOCK_HPP_

#include "includes.hpp"

namespace dyn{

class packed_block{

public:

	packed_block(){

		words = vector<uint64_t>();
		size_=0;
		psum_=0;

	}

	packed_block(vector<uint64_t>& words, uint32_t new_size, uint8_t width){

		this->words = vector<uint64_t>(words);
		this->size_= new_size;
		this->width_= width;
		this->int_per_word_ = 64/width_;

		MASK = (uint64_t(1) << width_)-1;

		psum_=psum(size_-1);

	}

	uint64_t at(uint64_t i){

		//assert(i<size_);

		return MASK & (words[i/int_per_word_] >> ((i%int_per_word_)*width_));

	}

	uint64_t psum(){

		return psum_;

	}

	/*
	 * inclusive partial sum (i.e. up to element i included)
	 */
	uint64_t psum(uint64_t i){

		assert(i<size_);

		i++;

		uint64_t s = 0;
		uint32_t pos = 0;

		// optimization for bitvectors

		for(uint32_t j = 0;j<(i/64)*(width_==1);++j){

			s += __builtin_popcountll(words[j]);
			pos += 64;

		}

		s += 	width_>1 or i%64==0 ?
				0 :
				__builtin_popcountll( words[i/64] & ((ulint(1) << (i%64))-1) );

		// end optimization for bitvectors

		for(uint32_t j=pos;j<i*(width_>1);++j){

			s += at(j);

		}

		return s;

	}

	/*
	 * smallest index j such that psum(j)>=x
	 */
	uint64_t search(uint64_t x){

		assert(size_>0);
		assert(x<=psum_);

		uint64_t s = 0;
		uint64_t pop = 0;
		uint32_t pos = 0;

		// optimization for bitvectors

		for(uint32_t j = 0; j < (size_/64)*(width_==1) and s < x;++j){

			pop = __builtin_popcountll(words[j]);
			pos += 64;
			s += pop;

		}

		// end optimization for bitvectors

		pos -= 64*(pos>0);
		s -= pop;

		for( ; pos<size_ and s<x;++pos){

			s += at(pos);

		}

		pos -= pos!=0;

		return pos;

	}

	/*
	 * this function works only for bitvectors, and
	 * is designed to support select_0. Returns first
	 * position i such that the number of zeros before
	 * i (included) is == x
	 */
	uint64_t search_0(uint64_t x){

		assert(size_>0);
		assert(width_==1);
		assert(x<=size_-psum_);

		uint64_t s = 0;
		uint64_t pop = 0;
		uint32_t pos = 0;

		// optimization for bitvectors

		for(uint32_t j = 0; j < size_/64 and s < x;++j){

			pop = 64-__builtin_popcountll(words[j]);
			pos += 64;
			s += pop;

		}

		// end optimization for bitvectors

		pos -= 64*(pos>0);
		s -= pop;

		for( ; pos<size_ and s<x;++pos){

			s += (1-at(pos));

		}

		pos -= pos!=0;

		return pos;

	}

	/*
	 * smallest index j such that psum(j)+j>=x
	 */
	uint64_t search_r(uint64_t x){

		assert(size_>0);
		assert(x<=psum_+size_);

		uint64_t s = 0;
		uint64_t pop = 0;
		uint32_t pos = 0;

		// optimization for bitvectors

		for(uint32_t j = 0; j < (size_/64)*(width_==1) and s < x;++j){

			pop = 64 + __builtin_popcountll(words[j]);
			pos += 64;
			s += pop;

		}

		// end optimization for bitvectors

		pos -= 64*(pos>0);
		s -= pop;

		for( ; pos<size_ and s<x;++pos){

			s += ( 1 + at(pos) );

		}

		pos -= pos!=0;

		return pos;

	}

	/*
	 * true iif x is one of the partial sums  0, I_0, I_0+I_1, ...
	 */
	bool contains(uint64_t x){

		assert(size_>0);
		assert(x<=psum_);

		uint64_t s = 0;

		for(uint32_t j=0;j<size_ and s<x;++j){

			s += at(j);

		}

		return s==x;

	}

	/*
	 * true iif x is one of  0, I_0+1, I_0+I_1+2, ...
	 */
	bool contains_r(uint64_t x){

		assert(size_>0);
		assert(x<=psum_+size_);

		uint64_t s = 0;

		for(uint32_t j=0;j<size_ and s<x;++j){

			s += (at(j)+1);

		}

		return s==x;

	}

	void increment(uint64_t i, uint64_t delta, bool subtract = false){

		assert(i<size_);

		//value at position i
		auto pvi = at(i);

		if(subtract){

			assert(pvi>=delta);
			set(i,pvi-delta);

			psum_ -= delta;

		}else{

			uint64_t s = pvi+delta;

			if(bitsize(s)>width_){

				//in this case rebuild the whole vector

				auto vec = to_vector();
				vec[i] += delta;

				rebuild(vec);

			}else{

				//just increment

				psum_ += delta;

				set(i,s);

			}

		}

	}

	void insert(uint64_t i, uint64_t x){

		if(bitsize(x)>width_){

			auto vec = to_vector(i,x);
			rebuild(vec);

			return;

		}

		//not enough space for the new element:
		//alloc extra_ new words
		if(size_+1>(words.size()*(int_per_word_))){

			//resize words

			auto temp = vector<uint64_t>(words.size()+extra_,0);

			uint64_t j = 0;
			for(auto x:words) temp[j++] = x;

			words = vector<uint64_t>(temp);

		}

		//shift right elements starting from number i
		shift_right(i);

		//insert x
		set(i,x);

		psum_+=x;
		size_++;

	}

	uint64_t size(){

		return size_;

	}

	/*
	 * returns total bit size of the structure
	 */
	uint64_t bit_size(){

		return 8*sizeof(packed_block) + words.capacity()*64;

	}

	vector<uint64_t> to_vector(){

		vector<uint64_t> vec(size_);

		uint32_t i = 0;
		for(uint32_t i=0;i<size_;++i) vec[i] = at(i);

		return vec;

	}

	/*
	 * split content of this vector into 2 packed blocks:
	 * Left part remains in this block, right part in the
	 * new returned block
	 */
	packed_block* split(){

		uint64_t prev_size = size_;

		uint32_t tot_words = (size_/int_per_word_) + (size_%int_per_word_!=0);

		assert(tot_words <= words.size());

		uint32_t nr_left_words = tot_words/2;
		uint32_t nr_right_words = tot_words-nr_left_words;

		assert(nr_left_words>0);
		assert(nr_right_words>0);

		uint32_t nr_left_ints = nr_left_words*int_per_word_;

		assert(size_ > nr_left_ints);
		uint32_t nr_right_ints = size_ - nr_left_ints;

		auto right_words = vector<uint64_t>(words.begin()+nr_left_words, words.begin()+tot_words);

		words = vector<uint64_t>(words.begin(), words.begin()+nr_left_words+extra_);

		size_ = nr_left_ints;
		psum_ = psum(size_-1);

		auto right = new packed_block(right_words,nr_right_ints,width_);

		return right;

	}

private:

	vector<uint64_t> to_vector(uint32_t j,uint64_t y){

		vector<uint64_t> vec(size_+1);

		uint32_t i = 0;
		for(uint32_t k=0;k<size_;++k){

			if(k==j) vec[i++] = y;

			vec[i++] = at(k);

		}

		if(j==size_) vec[size_] = y;

		return vec;

	}

	//shift right of 1 position elements starting
	//from the i-th.
	//assumption: last element does not overflow!
	void shift_right(uint32_t i){

		//number of integers that fit in a memory word
		assert(int_per_word_>0);

		assert(size_+1 <= words.size()*int_per_word_);

		uint32_t current_word = i/int_per_word_;

		uint32_t falling_out_idx = current_word*int_per_word_+(int_per_word_-1);

		//integer that falls out from the right of current word
		uint64_t falling_out = at(falling_out_idx);

		for(uint32_t j = falling_out_idx;j>i;--j) set(j,at(j-1));

		//now for the remaining integers we can work blockwise

		uint64_t falling_out_temp;

		for(uint32_t j = current_word+1;j<words.size();++j){

			falling_out_temp = at( j*int_per_word_+(int_per_word_-1) );

			words[j] = words[j] << width_;

			assert(at(j*int_per_word_)==0);

			set(j*int_per_word_,falling_out);

			falling_out = falling_out_temp;

		}

	}

	void rebuild(vector<uint64_t>& vec){

		assert(vec.size()>0);

		psum_ = sum(vec);
		width_ = max_bitsize(vec);
		int_per_word_ = 64/width_;

		MASK = (uint64_t(1) << width_)-1;

		size_=vec.size();

		words = vector<uint64_t>( size_/int_per_word_ + (size_%int_per_word_ != 0) + extra_ );

		uint32_t i = 0;
		for(auto x:vec) set(i++, x);

	}

	void set(uint32_t i, uint64_t x){

		assert(bitsize(x)<=width_);

		uint32_t word_nr = i/int_per_word_;
		uint8_t pos = i%int_per_word_;

		//set to 0 i-th entry
		uint64_t MASK1 = ~(MASK<<(width_*pos));
		words[word_nr] &= MASK1;

		//insert x inside i-th position
		words[word_nr] |= (x<<(width_*pos));

	}

	uint8_t max_bitsize(vector<uint64_t> &vec){

		uint8_t max_b=bitsize(vec[0]);

		for(auto x:vec){

			uint8_t bs = bitsize(x);

			if(bs>max_b) max_b=bs;

		}

		return max_b;

	}

	uint64_t sum(vector<uint64_t> &vec){

		uint64_t res = 0;

		for(auto x:vec) res += x;

		return res;

	}

	uint8_t bitsize(uint64_t x){

		if(x==0) return 1;

		return 64 - __builtin_clzll(x);

	}

	vector<uint64_t> words;
	uint64_t psum_=0;
	uint64_t MASK=0;
	uint16_t size_=0;
	uint8_t width_=0;
	uint8_t int_per_word_=0;

	//when reallocating, reserve extra_ words of space to accelerate insert
	static const uint8_t extra_ = 2;

};

}

#endif /* INTERNAL_PACKED_BLOCK_HPP_ */
