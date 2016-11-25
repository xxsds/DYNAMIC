/*
 * succinct_bitvector.hpp
 *
 *  Created on: Oct 27, 2015
 *      Author: nico
 *
 *  succinct btvector
 *
 */

#ifndef INTERNAL_DYNAMIC_BITVECTOR_HPP_
#define INTERNAL_DYNAMIC_BITVECTOR_HPP_

#include "includes.hpp"

namespace dyn{

template <class Container> class bv_reference{

public:

	bv_reference(Container &c, uint64_t idx): _bv(c), _idx(idx) {}

    operator bool() {
        return _bv.at(_idx);
    }

    bv_reference const&operator=(bool v) const {

		_bv.set(_idx, v);

        return *this;
    }

    bv_reference const&operator=(bv_reference& ref) const {

		_bv.set(_idx, bool(ref));

        return *this;
    }

private:

	Container &_bv;
	uint64_t _idx;

};

template<class spsi_type>
class succinct_bitvector{

public:

    using bv_ref = bv_reference<succinct_bitvector>;

	/*
	 * create empty dynamic bitvector
	 */
	succinct_bitvector(){}

	/*
	 * number of bits in the bitvector
	 */
	uint64_t size(){

		return spsi_.size();

	}

	/*
	 * high-level access to the bitvector. Supports assign (operator=) and access
	 */
	bv_ref operator[](uint64_t i){

		assert(i<size());
		return { *this, i };

	}

	/*
	 * access
	 */
	bool at(uint64_t i){

		assert(i<size());
		return spsi_.at(i);

	}

	uint64_t select(uint64_t i, bool b = true){

		return b ? select1(i) : select0(i);

	}

	/*
	 * position of i-th bit not set. 0 =< i < rank(size(),0)
	 */
	uint64_t select0(uint64_t i){

		assert(i<rank0(size()));
		return spsi_.search_0(i+1);

	}

	/*
	 * position of i-th bit set. 0 =< i < rank(size(),1)
	 */
	uint64_t select1(uint64_t i){

		assert(i<rank1(size()));
		return spsi_.search(i+1);

	}

	/*
	 * number of bits equal to b before position i EXCLUDED
	 */
	uint64_t rank(uint64_t i, bool b = true){

		assert(i<=size());

		auto r1 = i==0 ? 0 : spsi_.psum(i-1);

		return  b ? r1 : i-r1 ;

	}

	/*
	 * number of bits equal to 0 before position i EXCLUDED
	 */
	uint64_t rank0(uint64_t i){

		assert(i<=size());
		return (i==0?0:i-spsi_.psum(i-1));

	}

	/*
	 * number of bits equal to 1 before position i EXCLUDED
	 */
	uint64_t rank1(uint64_t i){

		assert(i<=size());
		return (i==0?0:spsi_.psum(i-1));

	}

	/*
	 * total number of bits not set
	 */
	uint64_t rank0(){

		return rank0(size());

	}

	/*
	 * total number of bits set
	 */
	uint64_t rank1(){

		return rank1(size());


	}

	/*
	 * insert a bit b at position i
	 */
	void insert(uint64_t i, bool b){

		spsi_.insert(i,b);

	}

	/* append b at the end of the bitvector */
	void push_back(bool b){
		insert(size(),b);
	}

	/*
	 * insert a bit not set at position i
	 */
	void insert0(uint64_t i){

		insert(i,false);

	}

	/*
	 * insert a bit set at position i
	 */
	void insert1(uint64_t i){

		insert(i,true);

	}

	void push_front(bool b){

		insert(0,b);

	}

	/*
	 * sets i-th bit to value.
	 */
	void set(uint64_t i, bool value = true){

		spsi_[i] = value;

	}

	/*
	 * Total number of bits allocated in RAM for this structure
	 */
	uint64_t bit_size() {

		return sizeof(succinct_bitvector<spsi_type>)*8 + spsi_.bit_size();

	}

	ulint serialize(ostream &out){

		return spsi_.serialize(out);

	}

	void load(istream &in){

		spsi_.load(in);

	}

private:

	//underlying Searchable partial sum with inserts structure.
	//the spsi contains only integers 0 and 1
	spsi_type spsi_;

};

}

#endif /* INTERNAL_DYNAMIC_BITVECTOR_HPP_ */
