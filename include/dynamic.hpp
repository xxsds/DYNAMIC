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
#include "internal/compressed_string.hpp"
#include "internal/succinct_bitvector.hpp"
#include "rle_string.hpp"

namespace dyn{

/*
 * a succinct searchable partial sum with inserts implemented with cache-efficient
 * B trees. All integers in the same leaf have the same bit size.
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
 * run-length encoded bitvector. More space-efficient than rle_str with alphabet size=2
 */
//typedef rle_bitvector<gap_bv> rle_bv;

/*
 * RLE string implemented with a run-length encoded wavelet tree. Each
 * WT node is run-length encoded.
 */
typedef compressed_string<rle_str> wtrle_str;

/*
 * string implemented with a gap-compressed wavelet tree. Each
 * WT node is gap-compressed.
 */
typedef compressed_string<gap_bv> gap_str;



// ------------- STRUCTURES DESIGNED ONLY FOR DEBUGGING PURPOSES -------------



/*
 * dynamic bitvector with trivial implementation (test purposes)
 */
typedef succinct_bitvector<spsi_check<> > bv_check;

typedef compressed_string<bv_check> str_check;

typedef rle_string<bv_check, str_check> rle_str_check;

//typedef rle_string<bool, gap_bv, str_check> rle_bv_check;

}

#endif /* DYNAMIC_TYPEDEFS_HPP_ */
