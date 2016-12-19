/*
 * gap_bitvector.hpp
 *
 *  Created on: Oct 23, 2015
 *      Author: nico
 *
 *  dynamic gap-encoded bitvector. It is based on a searchable partial sum with inserts data structure.
 *
 *  the structure supports delete only for non-set bits.
 *
 */

#ifndef INTERNAL_GAP_BITVECTOR_HPP_
#define INTERNAL_GAP_BITVECTOR_HPP_

#include "includes.hpp"

namespace dyn{

template<class spsi_type>
class gap_bitvector{

public:

	/*
	 * create empty bitvector. parameter max_n is not used (legacy option)
	 */
	gap_bitvector(uint64_t max_n = 0){

		spsi_.insert(0,0);
		size_ = 0;

	}

	/*
	 * number of bits in the bitvector
	 */
	uint64_t size(){
		return size_;
	}

	/*
	 * access
	 */
	bool operator[](uint64_t i){
		return at(i);
	}

	/*
	 * access
	 */
	bool at(uint64_t i){

		assert(i<size());

		return i==0 ? rank1(1) : rank1(i+1)-rank1(i);

		//return spsi_.contains_r(i+1);

	}

	/*
	 * access
	 */
	bool access(uint64_t i){

		return at(i);

	}

	/*
	 * position of i-th bit not set. 0 =< i < rank(size(),0)
	 */
	uint64_t select0(uint64_t i){

		assert(i<rank0());

		//i = number of zeros before position of interest
		//spsi_.psum(i+1) 	= block of zeros to which the i-th zero belongs
		//					= number of 1s before position of interest
		return i + spsi_.search(i+1);

	}

	/*
	 * position of i-th bit set. 0 =< i < rank(size(),1)
	 */
	uint64_t select1(uint64_t i){

		assert(i<rank1());
		return spsi_.psum(i)+i;

	}

	/*
	 * position of i-th bit equal to b. 0 =< i < rank(size(),0)
	 */
	uint64_t select(uint64_t i, bool b= true){

		return b ? select1(i) : select0(i);

	}

	/*
	 * total number of bits set
	 */
	uint64_t rank1(){
		assert(bits_set_ == rank1(size()));
		return bits_set_;
	}

	/*
	 * total number of bits not set
	 */
	uint64_t rank0(){
		assert(size_ - bits_set_ == rank0(size()));
		return size_ - bits_set_;
	}

	/*
	 * number of bits equal to b before position i EXCLUDED
	 */
	uint64_t rank(uint64_t i, bool b = true){

		assert(i<=size());

		auto r1 = spsi_.search_r(i+1);
		return b ? r1 : i-r1;

	}

	/*
	 * number of bits equal to 0 before position i EXCLUDED
	 */
	uint64_t rank0(uint64_t i){

		assert(i<=size());
		assert(i>=spsi_.search_r(i+1));

		return i-spsi_.search_r(i+1);

	}

	/*
	 * number of bits equal to 1 before position i EXCLUDED
	 */
	uint64_t rank1(uint64_t i){

		assert(i<=size());
		return spsi_.search_r(i+1);

	}

	void push_back(bool b){

		insert(size(),b);

	}

	void push_front(bool b){

		insert(0,b);

	}

	/*
	 * insert bit b at position i
	 */
	void insert(uint64_t i, bool b){

		if(b) insert1(i);
		else insert0(i);

	}

	/*
	 * insert nr bits equal to 0 starting from position i
	 */
	void insert0(uint64_t i, uint64_t nr = 1){

		if(nr==0) return;

		uint64_t j = spsi_.search_r(i+1);
		spsi_[j] += nr;

		size_ += nr;

	}

	/*
	 * insert a bit set at position i
	 */
	void insert1(uint64_t i){

		//number of 1s before position i = integer to decrement in spsi
		uint64_t j = rank1(i);

		//number of zeros in the 0-block containing position i
		uint64_t cnt = spsi_[j];

		//new size of j-th counter
		uint64_t newsize =  i - ( j==0?0:select1(j-1)+1 );

		uint64_t delta = cnt-newsize;

		spsi_[j] -= delta;

		spsi_.insert(j+1,delta);

		size_++;
		bits_set_++;

	}

	/*
	 * delete nr zeroes starting from position i
	 * causes a failed assertion if range [i,i+nr-1] contains bits set!
	 */
	void delete0(uint64_t i, uint64_t nr = 1){

		assert(i+nr<=size_);
		assert(rank1(i+nr)-rank1(i)==0);

		uint64_t j = spsi_.search_r(i+1);
		spsi_[j] -= nr;

		size_ -= nr;

	}

	/*
	 * sets i-th bit to 1. (does nothing if i-th bit is already set)
	 */
	void set(uint64_t i){

		if(at(i)) return;

		//number of 1s before position i = integer to decrement in spsi
		uint64_t j = rank1(i);

		//number of zeros in the 0-block containing position i
		uint64_t cnt = spsi_[j];

		//new size of j-th counter
		uint64_t newsize =  i - ( j==0?0:select1(j-1)+1 );

		uint64_t delta = cnt-newsize;

		spsi_[j] -= delta;

		assert(delta>0);

		spsi_.insert(j+1,delta-1);

		bits_set_++;

	}

	/*
	 * Total number of bits allocated in RAM for this structure
	 */
	uint64_t bit_size() {

		return sizeof(gap_bitvector<spsi_type>)*8 + spsi_.bit_size();

	}

	/*
	 * returns the length of i-th run of zeros plus 1 (i.e.
	 * the leading 1 is taken into account). i.e. if
	 * i-th run is 10^n, this function returns n+1
	 */
	/*uint64_t gapAt(uint64_t i){

		assert(i+1<spsi_.size());

		return spsi_[i+1]+1;

	}*/

	ulint serialize(ostream &out){

		ulint w_bytes=0;

		out.write((char*)&size_,sizeof(size_));
		w_bytes += sizeof(size_);

		out.write((char*)&bits_set_,sizeof(bits_set_));
		w_bytes += sizeof(bits_set_);

		w_bytes += spsi_.serialize(out);

		return w_bytes;

	}

	void load(istream &in){

		in.read((char*)&size_,sizeof(size_));
		in.read((char*)&bits_set_,sizeof(bits_set_));

		spsi_.load(in);

	}


private:

	/*
	 * underlying SPSI
	 *
	 * internally, we encode a run 10^n with the number n
	 * we moreover add a '1' at the beginning in order to
	 * represent bitvectors that start with 0.
	 *
	 */
	spsi_type spsi_;

	uint64_t size_=0;		//total number of bits
	uint64_t bits_set_=0;	//total number of bits set

};


}

#endif /* INTERNAL_GAP_BITVECTOR_HPP_ */
