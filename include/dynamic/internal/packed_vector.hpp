// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

/*
 * packed_vector.hpp
 *
 *  Created on: Oct 21, 2015
 *      Author: nico
 */

#ifndef INTERNAL_PACKED_BLOCK_HPP_
#define INTERNAL_PACKED_BLOCK_HPP_

#include "dynamic/internal/includes.hpp"

namespace dyn{

   template <class Container> class pv_reference{

   public:

      pv_reference(Container &c, uint64_t idx): _pv(c), _idx(idx) {}

      operator uint64_t() {
	 return _pv.at(_idx);
      }

      pv_reference const&operator=(uint64_t v) const {

	 _pv.set(_idx, v);

	 return *this;
      }

      pv_reference const&operator=(pv_reference& ref) const {

	 _pv.set(_idx, uint64_t(ref));

	 return *this;
      }

      //++pv[i]
      pv_reference const&operator++() const {

	 _pv.increment(_idx,1);

	 return *this;

      }

      //pv[i]++
      pv_reference const operator++(int) const {

	 pv_reference copy(*this);

	 ++(*this);

	 return copy;

      }

      //--pv[i]
      pv_reference const&operator--() const {

	 _pv.increment(_idx,1,true);

	 return *this;

      }

      //pv[i]--
      pv_reference const operator--(int) const {

	 pv_reference copy(*this);

	 --(*this);

	 return copy;

      }

      pv_reference const&operator+=(uint64_t d) const {

	 _pv.increment(_idx,d);

	 return *this;

      }

      pv_reference const&operator-=(uint64_t d) const {

	 _pv.increment(_idx,d,true);

	 return *this;

      }

   private:

      Container &_pv;
      uint64_t _idx;

   };


   class packed_vector{

   public:

      using pv_ref = pv_reference<packed_vector>;

      /* new packed vector containing size integers (initialized to 0) of width bits each*/
      packed_vector(ulint size = 0, ulint width = 0){

	 /* if size > 0, then width must be > 0*/
	 assert(size == 0 or width > 0);
	 assert(width <= 64);

	 size_=size;
	 width_ = width;
	 psum_=0;

	 if(!width_)
           return;

	 int_per_word_ = 64/width_;
	 MASK = (uint64_t(1) << width_)-1;

	 words = vector<uint64_t>( size_/int_per_word_ +  ( size_%int_per_word_ != 0 ) );

	 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
	 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
            && "uninitialized non-zero values in the end of the vector");
      }

      packed_vector(vector<uint64_t>&& _words, uint64_t size, uint8_t width) {

        assert(width);

	 words = std::move(_words);
	 size_= size;
	 width_= width;
	 int_per_word_ = 64/width_;

	 MASK = (uint64_t(1) << width_)-1;

	 psum_=psum(size_-1);

	 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
	 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
            && "uninitialized non-zero values in the end of the vector");
      }

      virtual ~packed_vector() {}

      /*
       * high-level access to the vector. Supports assign, access,
       * increment (++, +=), decrement (--, -=)
       */
       pv_ref operator[](uint64_t i){

	 return { *this, i };

      }

      uint64_t at(uint64_t i) const {

	 assert(i<size_);

	 return MASK & (words[i/int_per_word_] >> ((i%int_per_word_)*width_));

      }

      uint64_t psum() const {

	 return psum_;

      }

      /*
       * inclusive partial sum (i.e. up to element i included)
       */
      uint64_t psum(uint64_t i) const {

	 assert(i<size_);

	 i++;

	 uint64_t s = 0;
	 uint64_t pos = 0;

	 // optimization for bitvectors

	 for(uint64_t j = 0;j<(i/64)*(width_==1);++j){

	    s += __builtin_popcountll(words[j]);
	    pos += 64;

	 }

	 s += 	width_>1 or i%64==0 ?
	    0 :
	    __builtin_popcountll( words[i/64] & ((ulint(1) << (i%64))-1) );

	 // end optimization for bitvectors

	 for(uint64_t j=pos;j<i*(width_>1);++j){

	    s += at(j);

	 }

	 return s;

      }

      /*
       * smallest index j such that psum(j)>=x
       */
      uint64_t search(uint64_t x) const {

	 assert(size_>0);
	 assert(x<=psum_);

	 uint64_t s = 0;
	 uint64_t pop = 0;
	 uint64_t pos = 0;

	 // optimization for bitvectors

	 for(uint64_t j = 0; j < (size_/64)*(width_==1) and s < x;++j){

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
      uint64_t search_0(uint64_t x) const {

	 assert(size_>0);
	 assert(width_==1);
	 assert(x<=size_-psum_);

	 uint64_t s = 0;
	 uint64_t pop = 0;
	 uint64_t pos = 0;

	 // optimization for bitvectors

	 for(uint64_t j = 0; j < size_/64 and s < x;++j){

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
      uint64_t search_r(uint64_t x) const {

	 assert(size_>0);
	 assert(x<=psum_+size_);

	 uint64_t s = 0;
	 uint64_t pop = 0;
	 uint64_t pos = 0;

	 // optimization for bitvectors

	 for(uint64_t j = 0; j < (size_/64)*(width_==1) and s < x;++j){

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
      bool contains(uint64_t x) const {

	 assert(size_>0);
	 assert(x<=psum_);

	 uint64_t s = 0;

	 for(uint64_t j=0;j<size_ and s<x;++j){

	    s += at(j);

	 }

	 return s==x;

      }

      /*
       * true iif x is one of  0, I_0+1, I_0+I_1+2, ...
       */
      bool contains_r(uint64_t x) const {

	 assert(size_>0);
	 assert(x<=psum_+size_);

	 uint64_t s = 0;

	 for(uint64_t j=0;j<size_ and s<x;++j){

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
		assert(bitsize(pvi-delta)<=width_);
	    set_without_psum_update(i,pvi-delta);

	    psum_ -= delta;

	 }else{

	    uint64_t s = pvi+delta;

	    if(bitsize(s)>width_){

	       //in this case rebuild the whole vector with a new bitsize
	       rebuild_set( i, s );

	    }else{

	       //just increment

	       psum_ += delta;

	       assert(bitsize(s)<=width_);
	       set_without_psum_update(i,s);

	    }

	 }

      }

      void append(uint64_t x){

	 push_back(x);

      }

     void remove(uint64_t i) {

		 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
		 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                    || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
                && "uninitialized non-zero values in the end of the vector");

         assert(i < size_);
         auto x = this->at(i);

		 if (width_>1) { // otherwise, cannot rebuild
			if (bitsize(x) == width_) {

				uint8_t max_b = 0;

				for(ulint j = 0; j < size_ ;++j){
				  if (j != i) {
					 auto x = this->at(j);

					 uint8_t bs = bitsize(x);

					 if(bs>max_b) max_b=bs;
				  }

				}

				//rebuild entire vector
				rebuild_rem( i, max_b );

				return;

			}
		 }

		 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
		 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                    || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
                && "uninitialized non-zero values in the end of the vector");

		 //shift ints left, from position i + 1 onwards
		 shift_left( i );

		 //set_without_psum_update(size_-1, 0);

		 --size_;
		 psum_ -= x;

		 while ( words.size() > size_ / int_per_word_  + (size_%int_per_word_!=0) + extra_ ) {
            words.pop_back();
		 }

		 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
		 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                    || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
                && "uninitialized non-zero values in the end of the vector");

      }
      
      void insert(uint64_t i, uint64_t x){

         if(i==size()){
            push_back(x);
            return;
         }

	 if(bitsize(x)>width_){

	    //auto vec = to_vector(i,x);
	    //rebuild(vec);
	    rebuild_ins( i, x );

	    return;

	 }

	 //not enough space for the new element:
	 //alloc extra_ new words
	 if(size_+1>(words.size()*(int_per_word_))){
        //resize words
        words.reserve(words.size() + extra_);
        words.resize(words.size() + extra_, 0);
	 }

	 //shift right elements starting from number i
	 shift_right(i);

	 //insert x
	 assert(bitsize(x)<=width_);
	 set_without_psum_update(i,x);

	 psum_+=x;
	 ++size_;

	 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
	 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
            && "uninitialized non-zero values in the end of the vector");
      }

      void insert_word(uint64_t i,
                       uint64_t word, uint8_t width, uint8_t n) {
        assert(i <= size());
        assert(n);
        assert(n * width <= sizeof(word) * 8);
        assert(width * n == 64 || (word >> width * n) == 0);

        if (n == 1) {
            // only one integer to insert
            insert(i, word);

        } else if (width == 1 && width_ == 1 && n == 64) {
            // insert 64 bits packed into a word
            uint64_t pos = size_ / 64;
            uint8_t offset = size_ - pos * 64;

            if (!offset) {
                words.insert(words.begin() + pos, word);
            } else {
                assert(pos + 1 < words.size());

                words.insert(words.begin() + pos, words[pos + 1]);

                words[pos] &= ((1llu << offset) - 1);
                words[pos] |= word << offset;

                words[pos + 1] &= ~((1llu << offset) - 1);
                words[pos + 1] &= word >> (64 - offset);
            }

            size_ += n;
            psum_ += __builtin_popcountll(word);

        } else {
            const uint64_t mask = (1llu << width) - 1;
            while (n--) {
                insert(i++, word & mask);
                word >>= width;
            }
        }
      }

      /*
       * efficient push-back, implemented with a push-back on the underlying container
       * the insertion of an element whose bit-size exceeds the current width causes a
       * rebuild of the whole vector!
       */
      virtual void push_back(uint64_t x) {

	 if(bitsize(x)>width_){

	    //auto vec = to_vector(size(),x);
	    //rebuild(vec);
	    rebuild_ins(size(), x);

	    return;

	 }

	 //not enough space for the new element:
	 //push back a new word
	 if(size_+1>(words.size()*(int_per_word_)))
        words.push_back(0);

	 //insert x
	 assert(bitsize(x)<=width_);
	 set_without_psum_update(size(),x);

	 psum_+=x;
	 size_++;

	 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
	 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
            && "uninitialized non-zero values in the end of the vector");
      }

      uint64_t size() const {

	 return size_;

      }

	/*
	* split content of this vector into 2 packed blocks:
	* Left part remains in this block, right part in the
	* new returned block
	*/
	packed_vector* split(){

		 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
		 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                   || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
               && "uninitialized non-zero values in the end of the vector");

		uint64_t tot_words = (size_/int_per_word_) + (size_%int_per_word_!=0);

		assert(tot_words <= words.size());

		uint64_t nr_left_ints = size_ / 2 + (size_%2 != 0);
		uint64_t nr_right_ints = size_ - nr_left_ints;

		uint64_t nr_left_words = (nr_left_ints/int_per_word_) + (nr_left_ints%int_per_word_!=0);
		uint64_t nr_right_words = tot_words-nr_left_words;

		assert(nr_left_words>0);
		assert(nr_right_words>0);

		assert(size_ > nr_left_ints);
		assert(words.begin() + nr_left_words + extra_ < words.end());
		assert(words.begin() + tot_words <= words.end());

		assert(int_per_word_ == 64 / width_);

		auto right = new packed_vector(nr_right_ints, width_);

		for(uint64_t i = nr_left_ints; i<size_;++i)
			right->operator[](i-nr_left_ints) = at(i);

		size_ = nr_left_ints;
		psum_ = psum(size_-1);

		//clear unused bits
		words.resize(nr_left_words + extra_);
		std::fill(words.begin() + nr_left_words, words.end(), 0);
		words.shrink_to_fit();
		words[size_ / int_per_word_] &= ((~uint64_t(0)) >> (64 - ((size_ % int_per_word_) * width_)));

		 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
		 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                   || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
               && "uninitialized non-zero values in the end of the vector");

		return right;

      }



      /* set i-th element to x. updates psum */
      void set(uint64_t i, uint64_t x){

	 assert(bitsize(x) <= width_);
        assert(i < size_);

	 auto y = at(i);

	 psum_ = x<y ? psum_ - (y-x) : psum_ + (x-y);

	 uint64_t word_nr = i/int_per_word_;
	 uint8_t pos = i - int_per_word_ * word_nr;

	 //set to 0 i-th entry
	 uint64_t MASK1 = ~(MASK<<(width_*pos));
	 words[word_nr] &= MASK1;

	 //insert x inside i-th position
	 words[word_nr] |= (x<<(width_*pos));

	 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
	 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
            && "uninitialized non-zero values in the end of the vector");
      }

      /*
       * return total number of bits occupied in memory by this object instance
       */
      ulint bit_size() const {

	 return ( sizeof(packed_vector) + words.capacity()*sizeof(ulint) )*8;

      }

      ulint serialize(ostream &out) const {

	 ulint w_bytes = 0;

	 ulint w_size = words.size();

	 out.write((char*)&w_size,sizeof(w_size));
	 w_bytes += sizeof(w_size);

	 if(w_size>0){

	    out.write((char*)words.data(),sizeof(uint64_t)*w_size);
	    w_bytes += sizeof(uint64_t)*w_size;

	 }

	 out.write((char*)&psum_,sizeof(psum_));
	 w_bytes += sizeof(psum_);

	 out.write((char*)&MASK,sizeof(MASK));
	 w_bytes += sizeof(MASK);

	 out.write((char*)&size_,sizeof(size_));
	 w_bytes += sizeof(size_);

	 out.write((char*)&width_,sizeof(width_));
	 w_bytes += sizeof(width_);

	 out.write((char*)&int_per_word_,sizeof(int_per_word_));
	 w_bytes += sizeof(int_per_word_);

	 return w_bytes;

      }

      void load(istream &in){

	 ulint w_size;

	 in.read((char*)&w_size,sizeof(w_size));

	 if(w_size>0){

	    words = vector<uint64_t>(w_size);
	    in.read((char*)words.data(),sizeof(uint64_t)*w_size);

	 }

	 in.read((char*)&psum_,sizeof(psum_));

	 in.read((char*)&MASK,sizeof(MASK));

	 in.read((char*)&size_,sizeof(size_));

	 in.read((char*)&width_,sizeof(width_));

	 in.read((char*)&int_per_word_,sizeof(int_per_word_));

	 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
	 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
            && "uninitialized non-zero values in the end of the vector");
      }

      uint64_t width() const {
	 return width_;
      }
      
   protected:

     virtual void set_without_psum_update(uint64_t i, uint64_t x) {

	 assert(bitsize(x)<=width_);

	 uint64_t word_nr = i/int_per_word_;
	 uint8_t pos = i - int_per_word_ * word_nr;

	 //set to 0 i-th entry
	 uint64_t MASK1 = ~(MASK<<(width_*pos));
	 words[word_nr] &= MASK1;

	 //insert x inside i-th position
	 words[word_nr] |= (x<<(width_*pos));

      }

      void set_without_psum_update(uint64_t i, uint64_t x, vector< uint64_t >& new_words, uint8_t& new_int_per_word_, uint8_t& new_width_, uint64_t& new_MASK ){

	 assert(bitsize(x)<=new_width_);

	 uint64_t word_nr = i/new_int_per_word_;
	 uint8_t pos = i - new_int_per_word_ * word_nr;

	 //set to 0 i-th entry
	 uint64_t MASK1 = ~(new_MASK<<(new_width_*pos));
	 new_words[word_nr] &= MASK1;

	 //insert x inside i-th position
	 new_words[word_nr] |= (x<<(new_width_*pos));

      }

      packed_vector to_vector(uint64_t j,uint64_t y){

	 auto w2 = std::max(width_,bitsize(y));
	 auto vec = packed_vector(size_+1,w2);

	 //vector<uint64_t> vec(size_+1);

	 uint64_t i = 0;
	 for(uint64_t k=0;k<size_;++k){

	    if(k==j) vec[i++] = y;
	    vec[i++] = at(k);

	 }

	 if(j==size_) vec[size_] = y;

	 return vec;

      }

      //shift right of 1 position elements starting
      //from the i-th.
      //assumption: last element does not overflow!
      void shift_right(uint64_t i) {
        assert(i < size_);
        //number of integers that fit in a memory word
        assert(int_per_word_>0);
        assert(size_+1 <= words.size()*int_per_word_);

        uint64_t current_word = i / int_per_word_;

        //integer that falls out from the right of current word
        uint64_t falling_out = 0;

        if (current_word * int_per_word_ < i) {

            falling_out = (words[current_word] >> (int_per_word_-1)*width_) & ((uint64_t(1)<<width_)-1);
    		assert(bitsize(falling_out)<=width_);

            uint64_t falling_out_idx
                = std::min(current_word * int_per_word_ + (int_per_word_ - 1), size_);

            for (uint64_t j = falling_out_idx; j > i; --j) {
                assert(j - 1 < size_);
        		assert(bitsize(at(j - 1))<=width_);
                set_without_psum_update(j, at(j - 1));
            }

            current_word++;
        }

        //now for the remaining integers we can work blockwise

        uint64_t falling_out_temp;

        for (uint64_t j = current_word; j <= size_ / int_per_word_; ++j) {

            assert(j < words.size());

            falling_out_temp = (words[j] >> (int_per_word_ - 1) * width_) & ((uint64_t(1)<<width_)-1);
    		assert(bitsize(falling_out_temp)<=width_);

            words[j] <<= width_;

            assert(j * int_per_word_ >= size_ || !at(j * int_per_word_));

    		assert(bitsize(falling_out)<=width_);
            set_without_psum_update(j * int_per_word_, falling_out);

            falling_out = falling_out_temp;
        }

      }

      //shift left of 1 position elements starting
      //from the (i + 1)-st.
      void shift_left(uint64_t i){

 		 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
 		 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                     || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
                 && "uninitialized non-zero values in the end of the vector");


	 //number of integers that fit in a memory word
	 assert(int_per_word_>0);
     assert(i < size_);

	 if (i == (size_ - 1)) {
	    set_without_psum_update( i, 0 );

	    return;
	 }
	 
        uint64_t current_word = i / int_per_word_;

        //integer that falls in from the right of current word
        uint64_t falling_in_idx;

        if (current_word * int_per_word_ < i) {
            falling_in_idx = std::min((current_word + 1) * int_per_word_, size_ - 1);

            for(uint64_t j = i; j < falling_in_idx; ++j) {
                assert(j + 1 < size_);
        		assert(bitsize(at(j + 1))<=width_);
                set_without_psum_update(j, at(j + 1));
            }

            if (falling_in_idx == size_ - 1)
                set_without_psum_update(size_ - 1, 0);

            current_word++;
        }

        //now for the remaining integers we can work blockwise
        for (uint64_t j = current_word; j * int_per_word_ < size_; ++j) {
            words[j] >>= width_;

            falling_in_idx = (j + 1) * int_per_word_ < size_
                                ? at((j + 1) * int_per_word_)
                                : 0;

    		assert(bitsize(falling_in_idx)<=width_);
            set_without_psum_update(j * int_per_word_ + int_per_word_ - 1, falling_in_idx);
        }


		 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
		 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                   || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
               && "uninitialized non-zero values in the end of the vector");


      }

      // Rebuilds entire vector, setting j to value y
      // Should only be called when the bitsize of y exceeds
      // the current width
      void rebuild_set( uint64_t j, uint64_t y ) {
	 uint8_t new_width_ = std::max(width_,bitsize(y));
	 uint64_t new_psum_ = 0;
	 uint8_t new_int_per_word_ = 64/new_width_;
	 uint64_t new_size_= size_; 

	 vector< uint64_t > new_words( new_size_/new_int_per_word_ + (new_size_%new_int_per_word_ != 0) + extra_, 0 );
	 //vector< uint64_t > new_words( new_size_/new_int_per_word_ + (new_size_%new_int_per_word_ != 0) );

	 uint64_t new_MASK = (uint64_t(1) << new_width_)-1;

	 uint64_t i = 0;
	 for ( uint64_t k = 0; k < size_; ++k ) {
	    if ( k == j ) {
	       set_without_psum_update( i, y,
					new_words,
					new_int_per_word_,
					new_width_,
					new_MASK );
	       ++i;
	       new_psum_ += y;
	    } else {
              assert( k < size_ );
	       auto x = this->at( k );
	       set_without_psum_update( i, x,
					new_words,
					new_int_per_word_,
					new_width_,
					new_MASK );
	       ++i;
	       new_psum_ += x;
	    }
	 }
	 words.swap( new_words );
	 psum_ = new_psum_;
	 MASK = new_MASK;
	 size_ = new_size_;
	 width_ = new_width_;
	 int_per_word_ = new_int_per_word_;

	 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
	 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
            && "uninitialized non-zero values in the end of the vector");
      }

      // Rebuilds entire vector, removing j from the vector
      void rebuild_rem( uint64_t j, uint8_t new_width_ ) {
	 if ( (new_width_ == 0) || (size_ - 1 == 0)) {
	    width_ = 0;
	    size_ = 0;
	    words.clear();
	    MASK = 0;
	    int_per_word_ = 0;
	    psum_ = 0;

		 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
		 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                   || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
               && "uninitialized non-zero values in the end of the vector");
	    return;
	 }
	 
	 uint64_t new_psum_ = 0;
	 uint8_t new_int_per_word_ = 64/new_width_;
	 uint64_t new_size_= size_ - 1; 

	 vector< uint64_t > new_words( new_size_/new_int_per_word_ + (new_size_%new_int_per_word_ != 0) + extra_, 0 );

	 uint64_t new_MASK = (uint64_t(1) << new_width_)-1;

	 uint64_t i = 0;
	 for ( uint64_t k = 0; k < size_; ++k ) {
	    if ( k == j ) {
	       //skip
	       
	    } else {
              assert( k < size_ );
	       auto x = this->at( k );
	       set_without_psum_update( i, x,
					new_words,
					new_int_per_word_,
					new_width_,
					new_MASK );
	       ++i;
	       new_psum_ += x;
	    }
	 }
	 words.swap( new_words );
	 psum_ = new_psum_;
	 MASK = new_MASK;
	 size_ = new_size_;
	 width_ = new_width_;
	 int_per_word_ = new_int_per_word_;

	 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
	 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
            && "uninitialized non-zero values in the end of the vector");
      }

      // Rebuilds entire vector, inserting y at position j
      void rebuild_ins( uint64_t j, uint64_t y ) {
	 uint8_t new_width_ = std::max(width_,bitsize(y));
	 uint64_t new_psum_ = 0;
	 uint8_t new_int_per_word_ = 64/new_width_;
	 uint64_t new_size_= size_ + 1;

	 vector< uint64_t > new_words( new_size_/new_int_per_word_ + (new_size_%new_int_per_word_ != 0) + extra_, 0 );

	 uint64_t new_MASK = (uint64_t(1) << new_width_)-1;

	 uint64_t i = 0;
	 for ( uint64_t k = 0; k < size_; ++k ) {
	    if ( k == j ) {
	       set_without_psum_update( i, y,
					new_words,
					new_int_per_word_,
					new_width_,
					new_MASK );
	       ++i;
	       new_psum_ += y;
	    }

           assert( k < size_ );
	    auto x = this->at( k );
	    set_without_psum_update( i, x,
				     new_words,
				     new_int_per_word_,
				     new_width_,
				     new_MASK );
	    ++i;
	    new_psum_ += x;
	 }

	 if (j == size_) {
	    set_without_psum_update( size_, y,
				     new_words,
				     new_int_per_word_,
				     new_width_,
				     new_MASK );
	    new_psum_ += y;
	 }

	 psum_ = new_psum_;
	 MASK = new_MASK;
	 size_ = new_size_;
	 width_ = new_width_;
	 int_per_word_ = new_int_per_word_;

	 words.assign( new_words.begin(), new_words.end() );

	 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
	 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
            && "uninitialized non-zero values in the end of the vector");
      }
      
      void rebuild(packed_vector& vec){

	 assert(vec.size()>0);

	 psum_ = sum(vec);
	 width_ = max_bitsize(vec);
	 int_per_word_ = 64/width_;

	 MASK = (uint64_t(1) << width_)-1;

	 size_=vec.size();

	 words = vector<uint64_t>( size_/int_per_word_ + (size_%int_per_word_ != 0) + extra_ );

	 for(ulint j=0;j<vec.size();++j){

	    auto x = vec[j];
		assert(bitsize(x)<=width_);
	    set_without_psum_update(j, x);

	 }

	 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
	 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
            && "uninitialized non-zero values in the end of the vector");

      }

      uint8_t max_bitsize(packed_vector &vec){

	 uint8_t max_b=bitsize(vec[0]);

	 for(ulint i=0;i<vec.size();++i){

	    auto x = vec[i];

	    uint8_t bs = bitsize(x);

	    if(bs>max_b) max_b=bs;

	 }

	 return max_b;

      }

      uint64_t sum(packed_vector &vec){

	 uint64_t res = 0;

	 for(ulint i=0;i<vec.size();++i){

	    auto x = vec[i];
	    res += x;

	 }

	 return res;

      }

      uint8_t bitsize(uint64_t x){

	 if(x==0) return 1;

	 return 64 - __builtin_clzll(x);

      }

      vector<uint64_t> words;
      uint64_t psum_=0;
      uint64_t MASK=0;
      uint64_t size_=0;
      uint8_t width_=0;
      uint8_t int_per_word_=0;

      //when reallocating, reserve extra_ words of space to accelerate insert
      static const uint8_t extra_ = 2;

   };


class packed_bit_vector : public packed_vector {
public:
    explicit packed_bit_vector(ulint size = 0) : packed_vector(size, 1) {}

    packed_bit_vector(vector<uint64_t>&& words, uint64_t size)
        : packed_vector(std::move(words), size, 1) {}

    virtual void push_back(uint64_t x) override final {
        assert(int_per_word_ == 64);
        assert(size_ <= words.size() * 64);

        //not enough space for the new element:
        //push back a new word
        if (size_++ / 64 == words.size())
            words.push_back(0);

        assert(size_ <= words.size() * 64);
        assert(!at(size_ - 1));

        if (x) {
            //insert x at the last position
            words[(size_ - 1) / int_per_word_] |= static_cast<uint64_t>(1) << ((size_ - 1) % 64);
            psum_++;
        }
		 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
		 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                   || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
               && "uninitialized non-zero values in the end of the vector");
    }

    virtual void set_without_psum_update(uint64_t i, uint64_t x) override final {
        assert(bitsize(x)<=width_);
        assert(width_ == 1u);

        if (x) {
            words[i / 64] |= static_cast<uint64_t>(1) << (i % 64);
        } else {
            words[i / 64] &= ~(static_cast<uint64_t>(1) << (i % 64));
        }
    }


    packed_bit_vector* split() {
        uint64_t tot_words = (size_/int_per_word_) + (size_%int_per_word_!=0);

        assert(tot_words <= words.size());

        uint64_t nr_left_words = tot_words/2;

        assert(nr_left_words>0);
        assert(tot_words-nr_left_words>0);

        uint64_t nr_left_ints = nr_left_words*int_per_word_;

        assert(size_ > nr_left_ints);
        uint64_t nr_right_ints = size_ - nr_left_ints;

        assert(words.begin() + nr_left_words + extra_ < words.end());
        assert(words.begin() + tot_words <= words.end());
        vector<uint64_t> right_words(tot_words - nr_left_words + extra_, 0);
        std::copy(words.begin() + nr_left_words, words.begin() + tot_words, right_words.begin());
        words.resize(nr_left_words + extra_);
        std::fill(words.begin() + nr_left_words, words.end(), 0);
        words.shrink_to_fit();

        size_ = nr_left_ints;
        psum_ = psum(size_-1);

        auto right = new packed_bit_vector(std::move(right_words), nr_right_ints);

		 assert(size_ / int_per_word_ + (size_%int_per_word_!=0) <= words.size());
		 assert((size_ / int_per_word_ + (size_%int_per_word_!=0) == words.size()
                   || !(words[words.size()-1] >> ((size_ % int_per_word_) * width_)))
               && "uninitialized non-zero values in the end of the vector");

        return right;
    }
};

}

#endif /* INTERNAL_PACKED_BLOCK_HPP_ */
