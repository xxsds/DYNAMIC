/*
 * sparse_vector.hpp
 *
 *  Created on: Dec 24, 2015
 *      Author: nico
 *
 *
 *  encodes a vector of n 64-bits unsigned integers, m of which are > 0.
 *  The vector takes <= m*k + O(m log n/m) bits of space, where k is the maximum
 *  number of bits of any integer > 0.
 *
 *  The class supports the following operations:
 *
 *  V[i] : 	return a reference to i-th integer. Beware that the assignment V[i] = 0, where V[i] > 0
 *  		is not (yet) supported! (sorry...)
 *
 *  V.exists_nonNIL({l,r}) : true iff there exists a nonNIL integer in the range [l,r)
 *
 *  V.find_nonNIL({l,r}) :		returns 0 if no integer > 0 exists in the interval [l,r).
 *  							Otherwise, returns V[i], where l <= i < r is the smallest index
 *  							such that V[i] > 0
 *
 *	V.update_interval(j,k,{l,r}) :  note: for efficiency reasons, this procedure is defined only for
 *									nonempty intervals.
 *									Three cases:
 *									1. If there are no integers > 0 in [l,r), assign V[k] = j
 *									2. If there is only one integer > 0 in [l,r), assign V[k] = j
 *									3. 	Let k' < k'' be, resp.,  the smallest and largest indexes
 *										inside [l,r) such that V[k']>0 and V[k''] > 0. Three cases:
 *
 *										3.1 If k < k', assign V[k] = j and reset V[k'] = 0. This also
 *											frees the memory of V[k']
 *										3.2 If k > k'', assign V[k] = j and reset V[k''] = 0. This also
 *											frees the memory of V[k'']
 *										3.3 Otherwise, do nothing.
 *
 */

#ifndef INCLUDE_INTERNAL_SPARSE_VECTOR_HPP_
#define INCLUDE_INTERNAL_SPARSE_VECTOR_HPP_

#include "includes.hpp"
#include <gap_bitvector.hpp>


namespace dyn{

template <class Container> class sv_reference{

public:

	sv_reference(Container &c, uint64_t idx): _sv(c), _idx(idx) {}

    operator uint64_t() {
        return _sv.at(_idx);
    }

    sv_reference const&operator=(uint64_t v) const {

		_sv.set(_idx, v);

        return *this;
    }

    sv_reference const&operator=(sv_reference& ref) const {

		_sv.set(_idx, uint64_t(ref));

        return *this;
    }

private:

	Container &_sv;
	uint64_t _idx;

};


template<	class spsi_type,	// searchable partial sum with inserts
			class gap_bv_type>	// gap-encoded bitvector
class sparse_vector{

public:

    using sv_ref = sv_reference<sparse_vector>;

	/*
	 * constructor: create a new sparse vector containing n>0 NILs.
	 * Default: n = 2^64 - 1.
	 *
	 * the value NIL is reserved and cannot be inserted.
	 *
	 * The user can decide the value of the NIL. Default is 2^64 - 1
	 */
	sparse_vector(ulint n = 0, ulint NIL = ~ulint(0)){

		this->NIL = NIL;

		//insert n NILs
		if(n>0) bv_.insert0(0,n);

	}

	/*
	 * high-level access to the sparse vector. Supports assign and access.
	 */
	sv_ref operator[](uint64_t i){

		return { *this, i };

	}

	/*
	 * return i-th element
	 */
	ulint at(ulint i){

		assert(i<size());

		return bv_[i] ? spsi_[bv_.rank1(i)] : NIL;

	}

	/*
	 * set i-th element to x.
	 */
	void set(ulint i, ulint x){

		assert(i<size());

		/*
		 * reset elements not supported (yet)!
		 */
		assert(x != NIL);

		if(bv_[i]){

			spsi_[bv_.rank1(i)] = x;

		}else{

			spsi_.insert(bv_.rank1(i),x);
			bv_.set(i);

		}

	}

	/*
	 * number of non-NIL elements before position i
	 * excluded
	 */
	ulint rank(ulint i){

		assert(i<=size());
		return bv_.rank1(i);

	}

	/*
	 * insert x at the i-th position
	 */
	void insert(ulint i, ulint x){

		assert(i<=size());

		if(x==NIL){

			bv_.insert(i,false);

		}else{

			bv_.insert(i,true);
			spsi_.insert( bv_.rank1(i), x);

		}

	}

	void insert_NIL(ulint i){
		insert(i,get_NIL());
	}

	/*
	 * true iff there exists a non-NIL integer in the input range [l,r)
	 */
	bool exists_non_NIL(pair<ulint,ulint> range){

		auto l = range.first;
		auto r = range.second;

		assert(l <= size());
		assert(r <= size());

		return r <= l ? false : bv_.rank1(r) - bv_.rank1(l) > 0;

	}

	/* returns NIL if no integer != NIL exists in the interval [l,r).
	 * Otherwise, returns V[i], where l <= i < r is the smallest index
	 * such that V[i] != NIL
	 */
	ulint find_non_NIL(pair<ulint,ulint> range){

		auto l = range.first;
		auto r = range.second;

		assert(l <= size());
		assert(r <= size());

		ulint rl = bv_.rank1(l);

		bool exists = r <= l ? false : bv_.rank1(r) - rl > 0;

		return exists ? spsi_[rl] : NIL;

	}

	void update_interval(ulint j, ulint k, pair<ulint,ulint> range){

		auto l = range.first;
		auto r = range.second;

		assert(l < size());
		assert(r <= size());
		assert(k < size());

		//k must be inside the interval
		assert(k>=l);
		assert(k<r);

		assert(j != NIL);

		assert(r>l);	//not defined for empty intervals

		ulint rr = bv_.rank1(r);
		ulint rl = bv_.rank1(l);

		assert(rr>=rl);

		ulint nr = rr - rl;	//number of non-NIL elements in the interval

		if(nr == 0 or nr == 1){

			set(k,j);

		}else{

			assert(rr>0);

			/*
			 * indexes of leftmost and rightmost non-NIL elements in range
			 */
			ulint k1 = bv_.select1(rl);
			ulint k2 = bv_.select1(rr-1);

			//the 2 elements must be distinct
			assert(k2>k1);

			bool left = k <= k1;
			bool right = k >= k2;

			/*
			 * if k1 < k < k2: do nothing
			 */
			if(not left and not right) return;

			if(left){

				//number of positions of which to move the '1' in bv_
				ulint delta = k1-k;

				bv_.delete0(k,delta);
				bv_.insert0(k+1,delta);

				spsi_[rl] = j;

			}else{

				//number of positions of which to move the '1' in bv_
				ulint delta = k-k2;

				bv_.insert0(k2,delta);
				bv_.delete0(k+1,delta);

				spsi_[rr-1] = j;

			}

		}

	}

	ulint size(){
		return bv_.size();
	}

	ulint number_of_nonNIL_elements(){
		return spsi_.size();
	}

	/*
	 * get value of NIL
	 */
	ulint get_NIL(){
		return NIL;
	}

	/*
	 * Total number of bits allocated in RAM for this structure
	 */
	ulint bit_size(){

		return sizeof(sparse_vector<spsi_type, gap_bv_type>)*8 + spsi_.bit_size() + bv_.bit_size();

	}

	ulint serialize(ostream &out){

		ulint w_bytes=0;

		out.write((char*)&NIL,sizeof(NIL));
		w_bytes += sizeof(NIL);

		w_bytes += spsi_.serialize(out);
		w_bytes += bv_.serialize(out);

		return w_bytes;

	}

	void load(istream &in){

		in.read((char*)&NIL,sizeof(NIL));

		spsi_.load(in);
		bv_.load(in);

	}

private:

	ulint NIL;
	spsi_type spsi_;
	gap_bv_type bv_;

};

}

#endif /* INCLUDE_INTERNAL_SPARSE_VECTOR_HPP_ */
