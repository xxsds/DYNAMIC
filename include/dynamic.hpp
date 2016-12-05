/*
 * dynamic.hpp
 *
 *  Created on: Oct 21, 2015
 *      Author: nico
 */

#ifndef DYNAMIC_TYPEDEFS_HPP_
#define DYNAMIC_TYPEDEFS_HPP_

#include <internal/spsi.hpp>
#include <internal/gap_bitvector.hpp>
#include <internal/spsi_check.hpp>
#include <internal/succinct_bitvector.hpp>
#include <internal/rle_string.hpp>
#include <internal/bwt.hpp>
#include <internal/sparse_vector.hpp>
#include <internal/packed_vector.hpp>
#include <internal/wt_string.hpp>
#include <internal/fm_index.hpp>

namespace dyn{

/*
 * a succinct searchable partial sum with inserts implemented with cache-efficient
 * B trees. Logarithmic-sized leaves
 */
typedef spsi<packed_vector,256,16> packed_spsi;

/*
 * a succinct searchable partial sum with inserts implemented with cache-efficient
 * B trees. Quadratic-log sized leaves
 */
typedef spsi<packed_vector,8192,16> succinct_spsi;


/*
 * dynamic gap-encoded bitvector
 */
typedef gap_bitvector<packed_spsi> gap_bv;

/*
 * dynamic succinct bitvector (about 1.1n bits)
 */
typedef succinct_bitvector<succinct_spsi> suc_bv;

/*
 * succinct/compressed dynamic string implemented with wavelet trees.
 * user can choose (at construction time) between fixed-length / gamma / Huffman encoding of characters.
 */
typedef wt_string<suc_bv> wt_str;

/*
 * run-length encoded (RLE) string. This string uses 1 sparse bitvector
 * for all runs, one dynamic string for run heads, and sigma sparse bitvectors (one per character)
 */
typedef rle_string<gap_bv, wt_str> rle_str;

/*
 * RLE string implemented with a run-length encoded wavelet tree. Each
 * WT node is run-length encoded. rle_str is much more efficient and
 * thus preferable
 */
typedef wt_string<rle_str> wtrle_str;

/*
 * string implemented with a gap-compressed wavelet tree. Each
 * WT node is gap-compressed.
 */
typedef wt_string<gap_bv> wtgap_str;

/*
 * succinct/compressed BWT (see description of com_str)
 */
typedef bwt<wt_str,rle_str> wt_bwt;

/*
 * run-length encoded BWT
 */
typedef bwt<rle_str,rle_str> rle_bwt;

/*
 * dynamic sparse vector: <= m*k + O(m log n/m) bits of space, where k is the maximum
 * number of bits of any integer > 0 and n is the total number of integers.
 */
typedef sparse_vector<packed_spsi,gap_bv> sparse_vec;

/*
 * dynamic succinct/entropy compressed FM index. BWT positions are
 * marked with a succinct bitvector
 *
 * ( n*H0 + n + (n/k)*log n )(1+o(1)) bits of space, where k is the SA sample rate
 *
 */
typedef fm_index<wt_bwt, suc_bv, packed_spsi> wt_fmi;

/*
 * dynamic run-length encoded FM index. BWT positions are
 * marked with a gap-encoded bitvector.
 *
 * ( 2*R*log(n/R) + R*H0 + (n/k)*log(n/k) + (n/k)*log n )(1+o(1)) bits of space, where
 * k is the SA sample rate and R is the number of runs in the BWT
 *
 */
typedef fm_index<rle_bwt, gap_bv, packed_spsi> rle_fmi;


// ------------- STRUCTURES DESIGNED ONLY FOR DEBUGGING PURPOSES -------------



/*
 * dynamic bitvector with trivial implementation (test purposes)
 */
typedef succinct_bitvector<spsi_check<> > bv_check;

typedef wt_string<bv_check> str_check;

typedef rle_string<bv_check, str_check> rle_str_check;



// ------------- template specializations ------------------------------------

/*
 * build structure given as input the BWT in string format
 * and the terminator character.
 *
 * Efficient constructor: pushes back runs
 *
 */
template<>
inline
void rle_bwt::build_from_string(string& bwt, char_type terminator, bool verbose){

	long int step = 1000000;	//print status every step characters
	long int last_step = 0;

	terminator_position = bwt.size();

	char_type c = bwt[0];
	ulint k=1;

	for(ulint i=1; i<bwt.size();++i){

		if(bwt[i] != c){

			if(c == terminator){

				//there must be only one terminator in the string
				assert(terminator_position == bwt.size());
				assert(k==1);

				terminator_position = i-1;

			}else{

				insert_in_F(c,k);
				L.insert(L.size(),c,k);

			}

			c = bwt[i];
			k = 1;

		}else{

			k++;

		}

		if(verbose){

			if(i>last_step+(step-1)){

				last_step = i;
				cout << " " << i << " characters processed ..." << endl;

			}

		}

	}

	//last run
	if(c == terminator){

		//there must be only one terminator in the string
		assert(terminator_position == bwt.size());
		assert(k==1);

		//terminator in last BWT position
		terminator_position = bwt.size() - 1;

	}else{

		insert_in_F(c,k);
		L.insert(L.size(),c,k);

	}

	assert(size() == bwt.size());
	assert(terminator_position != bwt.size());

}

template<>
inline
ulint rle_bwt::number_of_runs(){

	return L.number_of_runs()+1;

}

template<>
inline
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
inline
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
