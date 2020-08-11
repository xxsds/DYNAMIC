// Copyright (c) 2019, Nicola Prezza, Erik Garrison.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

/*
 * hacked_vector.hpp
 *
 *  Created on: Feb 18, 2019
 *      Author: Erik Garrison
 */

#ifndef INTERNAL_HACKED_BLOCK_HPP_
#define INTERNAL_HACKED_BLOCK_HPP_

#include "dynamic/internal/includes.hpp"

namespace dyn{

template <class Container> class hv_reference{

public:

    hv_reference(Container &c, uint64_t idx): _pv(c), _idx(idx) {}

    operator uint64_t() {
        return _pv.at(_idx);
    }

    hv_reference const&operator=(uint64_t v) const {
        auto val = _pv.at(_idx);
        _pv.increment(_idx, (val>v?val-v:v-val), v<val);
        return *this;
    }

    hv_reference const&operator=(hv_reference& ref) const {
        auto val = _pv.at(_idx);
        auto v = uint64_t(ref);
        _pv.increment(_idx, (val>v?val-v:v-val), v<val);
        return *this;
    }

    //++pv[i]
    hv_reference const&operator++() const {

        _pv.increment(_idx,1);

        return *this;

    }

    //pv[i]++
    hv_reference const operator++(int) const {

        hv_reference copy(*this);

        ++(*this);

        return copy;

    }

    //--pv[i]
    hv_reference const&operator--() const {

        _pv.increment(_idx,1,true);

        return *this;

    }

    //pv[i]--
    hv_reference const operator--(int) const {

        hv_reference copy(*this);

        --(*this);

        return copy;

    }

    hv_reference const&operator+=(uint64_t d) const {

        _pv.increment(_idx,d);

        return *this;

    }

    hv_reference const&operator-=(uint64_t d) const {

        _pv.increment(_idx,d,true);

        return *this;

    }

private:

    Container &_pv;
    uint64_t _idx;

};


class hacked_vector{

public:

    using pv_ref = hv_reference<hacked_vector>;

    /* new hacked vector containing size integers (initialized to 0) of width bits each*/
    hacked_vector(ulint size = 0, ulint width = 0){

        /* if size > 0, then width must be > 0*/
        assert(size == 0 or width > 0);
        assert(width <= 64);

        size_=size;
        width_ = width;

        if(width_>0){

            int_per_word_ = 64/width_;
            MASK = (uint64_t(1) << width_)-1;

        }

        if(size_==0)
            words = vector<uint64_t>();
        else
            words = vector<uint64_t>( size_/int_per_word_ +  ( size_%int_per_word_ != 0 ) );

    }

    hacked_vector(vector<uint64_t>& words, uint64_t new_size, uint8_t width){

        this->words = vector<uint64_t>(words);
        this->size_= new_size;
        this->width_= width;
        this->int_per_word_ = 64/width_;

        MASK = (uint64_t(1) << width_)-1;

    }

    /*
     * high-level access to the vector. Supports assign, access,
     * increment (++, +=), decrement (--, -=)
     */
    pv_ref operator[](uint64_t i){

        return { *this, i };

    }

    uint64_t at(uint64_t i) const {

        //assert(i<size_);

        return MASK & (words[i/int_per_word_] >> ((i%int_per_word_)*width_));

    }

    void increment(uint64_t i, uint64_t delta, bool subtract = false){

        assert(i<size_);

        //value at position i
        auto pvi = at(i);

        if(subtract){

            assert(pvi>=delta);
            set(i,pvi-delta);

        }else{

            uint64_t s = pvi+delta;

            if(bitsize(s)>width_){

                //in this case rebuild the whole vector with a new bitsize
                rebuild_set( i, s );

            }else{

                //just increment

                set(i,s);

            }

        }

    }

    void append(uint64_t x){

        insert(size(),x);

    }

    void remove(uint64_t i) {
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

        //shift ints left, from position i + 1 onwards
        shift_left( i );

	 
        while ( (words.size() - extra_ - 1)*(int_per_word_) >= size_ - 1 ) {
            //more than extra_ extra words, delete 
            words.pop_back();
            if (words.size() == 0)
                break;
        }

        --size_;

    }
      
    void insert(uint64_t i, uint64_t x){

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
            auto temp = vector<uint64_t>(words.size()+extra_,0);

            uint64_t j = 0;
            for(auto x:words) temp[j++] = x;

            words = vector<uint64_t>(temp);

        }

        //shift right elements starting from number i
        shift_right(i);

        //insert x
        set(i,x);

        ++size_;

    }

      

    /*
     * efficient push-back, implemented with a push-back on the underlying container
     * the insertion of an element whose bit-size exceeds the current width causes a
     * rebuild of the whole vector!
     */
    void push_back(uint64_t x){

        if(bitsize(x)>width_){

            //auto vec = to_vector(size(),x);
            //rebuild(vec);
            rebuild_ins(size(), x);

            return;

        }

        //not enough space for the new element:
        //push back a new word
        if(size_+1>(words.size()*(int_per_word_))) words.push_back(0);

        //insert x
        set(size(),x);

        size_++;

    }

    uint64_t size() const {

        return size_;

    }

    /*
     * split content of this vector into 2 hacked blocks:
     * Left part remains in this block, right part in the
     * new returned block
     */
    hacked_vector* split(){

        uint64_t tot_words = (size_/int_per_word_) + (size_%int_per_word_!=0);

        assert(tot_words <= words.size());

        uint64_t nr_left_words = tot_words/2;

        assert(nr_left_words>0);
        assert(tot_words-nr_left_words>0);

        uint64_t nr_left_ints = nr_left_words*int_per_word_;

        assert(size_ > nr_left_ints);
        uint64_t nr_right_ints = size_ - nr_left_ints;

        auto right_words = vector<uint64_t>(words.begin()+nr_left_words, words.begin()+tot_words);
        words = vector<uint64_t>(words.begin(), words.begin()+nr_left_words+extra_);

        size_ = nr_left_ints;

        auto right = new hacked_vector(right_words,nr_right_ints,width_);

        return right;

    }

    /*
     * return total number of bits occupied in memory by this object instance
     */
    ulint bit_size() const {

        return ( sizeof(hacked_vector) + words.capacity()*sizeof(ulint) )*8;

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

        in.read((char*)&MASK,sizeof(MASK));

        in.read((char*)&size_,sizeof(size_));

        in.read((char*)&width_,sizeof(width_));

        in.read((char*)&int_per_word_,sizeof(int_per_word_));

    }

    uint64_t width() const {
        return width_;
    }

    void set(uint64_t i, uint64_t x){

        assert(bitsize(x)<=width_);

        uint64_t word_nr = i/int_per_word_;
        uint8_t pos = i%int_per_word_;

        //set to 0 i-th entry
        uint64_t MASK1 = ~(MASK<<(width_*pos));
        words[word_nr] &= MASK1;

        //insert x inside i-th position
        words[word_nr] |= (x<<(width_*pos));

    }

    void set(uint64_t i, uint64_t x, vector< uint64_t >& new_words, uint8_t& new_int_per_word_, uint8_t& new_width_, uint64_t& new_MASK ){

        assert(bitsize(x)<=new_width_);

        uint64_t word_nr = i/new_int_per_word_;
        uint8_t pos = i%new_int_per_word_;

        //set to 0 i-th entry
        uint64_t MASK1 = ~(new_MASK<<(new_width_*pos));
        new_words[word_nr] &= MASK1;

        //insert x inside i-th position
        new_words[word_nr] |= (x<<(new_width_*pos));

    }

    hacked_vector to_vector(uint64_t j,uint64_t y){

        auto w2 = std::max(width_,bitsize(y));
        auto vec = hacked_vector(size_+1,w2);

        //vector<uint64_t> vec(size_+1);

        uint64_t i = 0;
        for(uint64_t k=0;k<size_;++k){

            if(k==j) vec[i++] = y;
            vec[i++] = at(k);

        }

        if(j==size_) vec[size_] = y;

        return vec;

    }

    void clear(void) {
        words.clear();
        MASK=0;
        size_=0;
        width_=0;
        int_per_word_=0;
    }

private:
    
    //shift right of 1 position elements starting
    //from the i-th.
    //assumption: last element does not overflow!
    void shift_right(uint64_t i){

        //number of integers that fit in a memory word
        assert(int_per_word_>0);

        assert(size_+1 <= words.size()*int_per_word_);

        uint64_t current_word = i/int_per_word_;

        uint64_t falling_out_idx = current_word*int_per_word_+(int_per_word_-1);

        //integer that falls out from the right of current word
        uint64_t falling_out = at(falling_out_idx);

        for(uint64_t j = falling_out_idx;j>i;--j) set(j,at(j-1));

        //now for the remaining integers we can work blockwise

        uint64_t falling_out_temp;

        for(uint64_t j = current_word+1;j<words.size();++j){

            falling_out_temp = at( j*int_per_word_+(int_per_word_-1) );

            words[j] = words[j] << width_;

            assert(at(j*int_per_word_)==0);

            set(j*int_per_word_,falling_out);

            falling_out = falling_out_temp;

        }

    }

    //shift left of 1 position elements starting
    //from the (i + 1)-st.
    void shift_left(uint64_t i){

        //number of integers that fit in a memory word
        assert(int_per_word_>0);

        if (i == (size_ - 1)) {
            set( i, 0 );

            return;
        }
	 
        uint64_t current_word = i/int_per_word_;

        //integer that falls in from the right of current word
        uint64_t falling_in_idx = (current_word + 1)*int_per_word_;
        uint64_t falling_in;
        if (falling_in_idx > (size_ - 1)) {
            //nothing falls in
            falling_in = 0;
            falling_in_idx = size_ - 1;
        }

	 
        for(uint64_t j = i; j <= falling_in_idx - 1;++j) {
            set(j,at(j+1));
        }

        //now for the remaining integers we can work blockwise
        for(uint64_t j = current_word+1;j<words.size();++j){
            words[j] = words[j] >> width_;
	    
            if (j < words.size() - 1) {
                falling_in = at( (j + 1)*int_per_word_ );
	    
                set(j*int_per_word_ + int_per_word_ - 1,falling_in);
            }

        }

    }

    // Rebuilds entire vector, setting j to value y
    // Should only be called when the bitsize of y exceeds
    // the current width
    void rebuild_set( uint64_t j, uint64_t y ) {
        uint8_t new_width_ = std::max(width_,bitsize(y));
        uint8_t new_int_per_word_ = 64/new_width_;
        uint64_t new_size_= size_; 

        vector< uint64_t > new_words( new_size_/new_int_per_word_ + (new_size_%new_int_per_word_ != 0) + extra_, 0 );
        //vector< uint64_t > new_words( new_size_/new_int_per_word_ + (new_size_%new_int_per_word_ != 0) );

        uint64_t new_MASK = (uint64_t(1) << new_width_)-1;

        uint64_t i = 0;
        for ( uint64_t k = 0; k < size_; ++k ) {
            if ( k == j ) {
                set( i, y,
                     new_words,
                     new_int_per_word_,
                     new_width_,
                     new_MASK );
                ++i;
            } else {
                auto x = this->at( k );
                set( i, x,
                     new_words,
                     new_int_per_word_,
                     new_width_,
                     new_MASK );
                ++i;
            }
        }
        words.swap( new_words );
        MASK = new_MASK;
        size_ = new_size_;
        width_ = new_width_;
        int_per_word_ = new_int_per_word_;
    }

    // Rebuilds entire vector, removing j from the vector
    void rebuild_rem( uint64_t j, uint8_t new_width_ ) {
        if ( (new_width_ == 0) || (size_ - 1 == 0)) {
            width_ = 0;
            size_ = 0;
            words.clear();
            MASK = 0;
            int_per_word_ = 0;

            return;
        }
	 
        uint8_t new_int_per_word_ = 64/new_width_;
        uint64_t new_size_= size_ - 1; 

        vector< uint64_t > new_words( new_size_/new_int_per_word_ + (new_size_%new_int_per_word_ != 0) + extra_, 0 );

        uint64_t new_MASK = (uint64_t(1) << new_width_)-1;

        uint64_t i = 0;
        for ( uint64_t k = 0; k < size_; ++k ) {
            if ( k == j ) {
                //skip
	       
            } else {
                auto x = this->at( k );
                set( i, x,
                     new_words,
                     new_int_per_word_,
                     new_width_,
                     new_MASK );
                ++i;
            }
        }
        words.swap( new_words );
        MASK = new_MASK;
        size_ = new_size_;
        width_ = new_width_;
        int_per_word_ = new_int_per_word_;
    }

    // Rebuilds entire vector, inserting y at position j
    void rebuild_ins( uint64_t j, uint64_t y ) {
        uint8_t new_width_ = std::max(width_,bitsize(y));
        uint8_t new_int_per_word_ = 64/new_width_;
        uint64_t new_size_= size_ + 1;

        vector< uint64_t > new_words( new_size_/new_int_per_word_ + (new_size_%new_int_per_word_ != 0) + extra_, 0 );

        uint64_t new_MASK = (uint64_t(1) << new_width_)-1;

        uint64_t i = 0;
        for ( uint64_t k = 0; k < size_; ++k ) {
            if ( k == j ) {
                set( i, y,
                     new_words,
                     new_int_per_word_,
                     new_width_,
                     new_MASK );
                ++i;
            }

            auto x = this->at( k );
            set( i, x,
                 new_words,
                 new_int_per_word_,
                 new_width_,
                 new_MASK );
            ++i;
        }

        if (j == size_) {
            set( size_, y,
                 new_words,
                 new_int_per_word_,
                 new_width_,
                 new_MASK );
        }

        MASK = new_MASK;
        size_ = new_size_;
        width_ = new_width_;
        int_per_word_ = new_int_per_word_;

        words.assign( new_words.begin(), new_words.end() );
    }
      
    void rebuild(hacked_vector& vec){

        assert(vec.size()>0);

        width_ = max_bitsize(vec);
        int_per_word_ = 64/width_;

        MASK = (uint64_t(1) << width_)-1;

        size_=vec.size();

        words = vector<uint64_t>( size_/int_per_word_ + (size_%int_per_word_ != 0) + extra_ );

        for(ulint j=0;j<vec.size();++j){

            auto x = vec[j];
            set(j, x);

        }

    }

    uint8_t max_bitsize(hacked_vector &vec){

        uint8_t max_b=bitsize(vec[0]);

        for(ulint i=0;i<vec.size();++i){

            auto x = vec[i];

            uint8_t bs = bitsize(x);

            if(bs>max_b) max_b=bs;

        }

        return max_b;

    }

    uint64_t sum(hacked_vector &vec){

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
    uint64_t MASK=0;
    uint64_t size_=0;
    uint8_t width_=0;
    uint8_t int_per_word_=0;

    //when reallocating, reserve extra_ words of space to accelerate insert
    static const uint8_t extra_ = 2;

};

}

#endif /* INTERNAL_HACKED_BLOCK_HPP_ */
