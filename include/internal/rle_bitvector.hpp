/*
 * rle_bitvector.hpp
 *
 *  Created on: Dec 1, 2015
 *      Author: nico
 */

#ifndef INCLUDE_INTERNAL_RLE_BITVECTOR_HPP_
#define INCLUDE_INTERNAL_RLE_BITVECTOR_HPP_

#include "includes.hpp"
#include <typeinfo>

namespace dyn{

template<
	class sparse_bitvector_t	//bitvector taking O(b) words of space,
								//b being the number of bits set
>
class rle_bitvector{

public:

	rle_bitvector(){}

	bool at(ulint i){

		assert(i<n);
		return 0;

	}

	bool operator[](ulint i){

		return at(i);

	}

	/*
	 * position of i-th character c. i starts from 0!
	 */
	ulint select(ulint i, bool b){

		return 0;

	}

	/*
	 * position of i-th 0. i starts from 0 (only for bitvectors!)
	 */
	ulint select0(ulint i){

		return select(i, false);

	}

	/*
	 * position of i-th 1. i starts from 0 (only for bitvectors!)
	 */
	ulint select1(ulint i){

		return select(i, true);

	}

	/*
	 * number of c before position i excluded
	 */
	ulint rank(ulint i, bool b){

		assert(i<=size());

		return 0;

	}

	/*
	 * number of 0s before position i (only for bitvectors!)
	 */
	ulint rank0(ulint i){

		return rank(i, false);

	}

	/*
	 * number of 1s before position i (only for bitvectors!)
	 */
	ulint rank1(ulint i){

		return rank(i, true);

	}

	/*
	 * insert character c at position i
	 */
	void insert(ulint i, bool b){



	}

	/*
	 * insert 0 at position i  (only for bitvectors!)
	 */
	void insert0(ulint i){

		insert(i,false);

	}

	/*
	 * insert 1 at position i  (only for bitvectors!)
	 */
	void insert1(ulint i){

		insert(i,true);

	}


	ulint size(){return n;}



	ulint number_of_runs(){return R;}



private:

	//bitvector of length n storing a 1 at the
	//end of each 0-run
	sparse_bitvector_t end_0_runs;

	//bitvector of length 'number of bits set' storing
	//a 1 at the end of each 1-run (1-runs are compacted)
	sparse_bitvector_t end_1_runs;

	//first bit of the bitvector
	bool first_bit=false;

	//length and number of runs
	ulint n=0;
	ulint R=0;

};

}

#endif /* INCLUDE_INTERNAL_RLE_BITVECTOR_HPP_ */
