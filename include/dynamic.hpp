/*
 * typedefs.hpp
 *
 *  Created on: Oct 21, 2015
 *      Author: nico
 */

#ifndef INTERNAL_TYPEDEFS_HPP_
#define INTERNAL_TYPEDEFS_HPP_

#include <spsi.hpp>
#include <packed_block.hpp>
#include <gap_bitvector.hpp>
#include <dynamic_bitvector.hpp>
#include <spsi_check.hpp>
#include <dynamic_string.hpp>

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
typedef dynamic_bitvector<spsi<packed_block,8192,16> > dyn_bv;


/*
 * dynamic bitvector with trivial implementation (test purposes)
 */
typedef dynamic_bitvector<spsi_check<> > dyn_bitv_check;

/*
 * succinct/compressed dynamic string
 */
typedef dynamic_string<dyn_bv> dyn_str;

}

#endif /* INTERNAL_TYPEDEFS_HPP_ */
