/*
 * dynamic.hpp
 *
 *  Created on: Oct 21, 2015
 *      Author: nico
 */

#ifndef DYNAMIC_TYPEDEFS_HPP_
#define DYNAMIC_TYPEDEFS_HPP_

#include <spsi.hpp>
#include <gap_bitvector.hpp>
#include <spsi_check.hpp>
#include <compressed_string.hpp>
#include <succinct_bitvector.hpp>
#include <rle_string.hpp>
#include <bwt.hpp>
#include <sparse_vector.hpp>
#include "internal/packed_vector.hpp"

namespace dyn{

/*
 * a succinct searchable partial sum with inserts implemented with cache-efficient
 * B trees.
 */
typedef spsi<packed_vector,256,16> packed_spsi;

/*
 * dynamic gap-encoded bitvector
 */
typedef gap_bitvector<packed_spsi> gap_bv;

/*
 * dynamic succinct bitvector (about 1.1n bits)
 */
typedef succinct_bitvector<spsi<packed_vector,8192,16> > suc_bv;

/*
 * succinct/compressed dynamic string implemented with wavelet trees.
 * user can choose (at construction time) between fixed-length / gamma / Huffman encoding of characters.
 */
typedef compressed_string<suc_bv> com_str;

/*
 * run-length encoded (RLE) string. This string uses 1 sparse bitvector
 * for all runs, one dynamic string for run heads, and sigma sparse bitvectors (one per character)
 */
typedef rle_string<gap_bv, com_str> rle_str;

/*
 * RLE string implemented with a run-length encoded wavelet tree. Each
 * WT node is run-length encoded. rle_str is much more efficient and
 * thus preferable
 */
typedef compressed_string<rle_str> wtrle_str;

/*
 * string implemented with a gap-compressed wavelet tree. Each
 * WT node is gap-compressed.
 */
typedef compressed_string<gap_bv> wtgap_str;

/*
 * succinct/compressed BWT (see description of com_str)
 */
typedef bwt<com_str,rle_str> com_bwt;

/*
 * run-length encoded BWT
 */
typedef bwt<rle_str,rle_str> rle_bwt;

/*
 * dynamic sparse vector: <= m*k + O(m log n/m) bits of space, where k is the maximum
 * number of bits of any integer > 0 and n is the total number of integers.
 */
typedef sparse_vector<packed_spsi,gap_bv> sparse_vec;


// ------------- STRUCTURES DESIGNED ONLY FOR DEBUGGING PURPOSES -------------



/*
 * dynamic bitvector with trivial implementation (test purposes)
 */
typedef succinct_bitvector<spsi_check<> > bv_check;

typedef compressed_string<bv_check> str_check;

typedef rle_string<bv_check, str_check> rle_str_check;



// ------------- template specializations ------------------------------------

template<>
ulint rle_bwt::number_of_runs(){

	return L.number_of_runs()+1;

}

template<>
ulint rle_bwt::number_of_runs(pair<ulint,ulint> interval){

	//coordinates on BWT with terminator
	auto l1 = interval.first;
	auto r1 = interval.second;

	//coordinates on BWT without terminator
	ulint l2 = 	interval.first <= terminator_position ?
				interval.first :
				interval.first-1;

	ulint r2 = 	interval.second <= terminator_position ?
				interval.second :
				interval.second-1;

	//if terminator falls outside the interval, just count number of runs
	//otherwise, take into account terminator
	return	terminator_position < l1 or terminator_position >= r1 ?
			L.number_of_runs({l2,r2}) :
				terminator_position == l1 or terminator_position == r1-1 ?
				L.number_of_runs({l2,r2})+1 :
					L[terminator_position-1] == L[terminator_position] ?
					L.number_of_runs({l2,r2})+2 :
					L.number_of_runs({l2,r2})+1;

}

/*
 * given a position i inside the string, return the interval [l,r) of the run containing i,
 * i.e. i \in [l,r) (right position always exclusive)
 */
template<>
pair<ulint,ulint> rle_bwt::locate_run(ulint i){

	assert(i<bwt_length());

	//if i is terminator position, its run is {i,i+1}
	if(i==terminator_position) return {i,i+1};

	//position on BWT without terminator
	ulint i1 = 	i <= terminator_position ?
				i :
				i-1;

	//range on BWT without terminator
	auto range = L.locate_run(i1);

	//adjust coordinates of the range:
	range.first = 	range.first < terminator_position ?
					range.first :
					range.first + 1;

	range.second = 	range.second < terminator_position ?
					range.second :
					range.second + 1;

	//if terminator falls inside the range, split the range

	return	terminator_position < range.first or terminator_position >= range.second ?
			range :							//terminator outside range
				i < terminator_position ? 	//terminator inside range
				pair<ulint,ulint> {range.first,terminator_position} :	//i at the left of terminator
				pair<ulint,ulint> {terminator_position+1,range.second};

}

}

#endif /* DYNAMIC_TYPEDEFS_HPP_ */
