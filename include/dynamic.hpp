/*
 * dynamic.hpp
 *
 *  Created on: Oct 21, 2015
 *      Author: nico
 */

#ifndef DYNAMIC_TYPEDEFS_HPP_
#define DYNAMIC_TYPEDEFS_HPP_

#include <spsi.hpp>
#include <packed_block.hpp>
#include <gap_bitvector.hpp>
#include <spsi_check.hpp>
#include <compressed_string.hpp>
#include <succinct_bitvector.hpp>
#include <rle_string.hpp>
#include <bwt.hpp>

namespace dyn{

/*
 * a succinct searchable partial sum with inserts implemented with cache-efficient
 * B trees.
 */
typedef spsi<packed_block,256,16> packed_spsi;

/*
 * dynamic gap-encoded bitvector
 */
typedef gap_bitvector<packed_spsi> gap_bv;

/*
 * dynamic succinct bitvector (about 1.1n bits)
 */
typedef succinct_bitvector<spsi<packed_block,8192,16> > suc_bv;

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


}

#endif /* DYNAMIC_TYPEDEFS_HPP_ */
