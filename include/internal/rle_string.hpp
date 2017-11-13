// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

/*
 * rle_string.hpp
 *
 *  Created on: Dec 1, 2015
 *      Author: nico
 *
 *  in the bitvectors, runs are encoded as 0^k1, k being the run length
 *
 */

#ifndef INCLUDE_INTERNAL_RLE_STRING_HPP_
#define INCLUDE_INTERNAL_RLE_STRING_HPP_

#include "includes.hpp"

namespace dyn{

template<
	class sparse_bitvector_t,	//bitvector taking O(b) words of space,
								//b being the number of bits set
	class string_t				//data structure implementing a string with
								//access/rank/select/insert functionalities.
>
class rle_string{

public:

	typedef ulint char_type;

	/*
	 * Constructor #1
	 *
	 * 2 cases:
	 * The string accepts any alphabet (unknown at priori). Run-heads are are gamma-coded
	 *
	 */
	rle_string(){}

	/*
	 * Constructor #2
	 *
	 * We know only alphabet size. Each Run-head char is assigned log2(sigma) bits.
	 * Characters are assigned codes 0,1,2,... in order of appearance
	 *
	 */
	rle_string(uint64_t sigma){

		assert(sigma>0);

		run_heads_ = string_t(sigma);

	}

	/*
	 * Constructor #3
	 *
	 * We know character probabilities. Input: pairs <character, probability>
	 *
	 * Here Run-heads are Huffman encoded.
	 *
	 */
	rle_string(vector<pair<char_type,double> >& P){

		run_heads_ = string_t(P);

	}

	char_type at(ulint i){

		assert(i<runs.size());
		assert(runs.rank1(i) < number_of_runs());
		return run_heads_[ runs.rank1(i) ];

	}

	char_type operator[](ulint i){

		return at(i);

	}

	bool char_exists(char_type c){

		return run_heads_.char_exists(c);

	}

	/*
	 * position of i-th character c. i starts from 0!
	 */
	ulint select(ulint i, char_type c){

		assert(run_heads_.char_exists(c));
		assert(i < rank(size(),c));

		ulint this_c_run = runs_per_letter[c].rank1(i);

		//position of i-th c inside its c-run
		ulint sel = i - ( this_c_run == 0 ? 0 : runs_per_letter[c].select1(this_c_run-1)+1 );

		//run number among all runs
		ulint this_run = run_heads_.select(this_c_run, c);

		sel += (this_run == 0 ? 0 : runs.select1(this_run-1)+1);

		return sel;

	}

	/*
	 * position of i-th bit not set. 0 =< i < rank(size(),0)
	 */
	uint64_t select0(uint64_t i){

		assert(run_heads_.char_exists(false));
		assert(i<rank0(size()));

		return select(i,false);

	}

	/*
	 * position of i-th bit set. 0 =< i < rank(size(),1)
	 */
	uint64_t select1(uint64_t i){

		assert(run_heads_.char_exists(true));
		assert(i<rank1(size()));

		return select(i,true);

	}

	/*
	 * number of c before position i excluded
	 */
	ulint rank(ulint i, char_type c){

		assert(i<=size());

		if(not run_heads_.char_exists(c)) return 0;

		//this run is the number 'this_run' among all runs
		ulint this_run = runs.rank1(i);

		assert(this_run <= run_heads_.size());

		//this c-run is the number 'this_c_run' among all c-runs
		ulint this_c_run = run_heads_.rank(this_run,c);

		//number of cs before position i (excluded) in THIS c-run
		ulint rk = (this_run == run_heads_.size() || run_heads_[this_run] != c) ?
					0 :
					i - (this_run == 0 ? 0 : runs.select1(this_run-1)+1 );

		assert(runs_per_letter[c].size()>0);
		assert(this_c_run == 0 || this_c_run-1 < runs_per_letter[c].rank1(runs_per_letter[c].size()));

		//add also number of cs before this run (excluded)
		rk += (this_c_run == 0 ? 0 : runs_per_letter[c].select1(this_c_run-1)+1 );

		return rk;

	}

	/*
	 * number of 0s before position i (only for bitvectors!)
	 */
	ulint rank0(ulint i){

		assert(run_heads_.char_exists(false));
		return rank(i, false);

	}

	/*
	 * number of 1s before position i (only for bitvectors!)
	 */
	ulint rank1(ulint i){

		assert(run_heads_.char_exists(true));
		return rank(i, true);

	}

	/*
	 * insert k copies of character c at position i
	 */
	void insert(ulint i, char_type c, ulint k = 1){

		assert(i<=size());
		assert(k>0);

		//CASE #1: empty string

		if(size()==0){

			assert(runs.size()==0);
			assert(runs_per_letter[c].size()==0);
			assert(number_of_runs()==0);

			runs.insert1(0);
			runs.insert0(0,k-1);

			run_heads_.insert(0,c);

			runs_per_letter[c].insert1(0);
			runs_per_letter[c].insert0(0,k-1);

			//increase length and number of runs
			//n++;
			//R++;

			assert(runs_per_letter[c].size() > 0);
			assert(at(i)==c);

			assert( run_at( runs.rank1(i) ) == run_at( run_heads_.rank(runs.rank1(i),c), c ) );
			return;

		}

		assert(size()>0);

		//c will be inserted between two caracters 'prev' and 'next'
		//if one of those 2 chars does not exist, the associated
		//variable (prev/next) is 0
		char_type prev = (i == 0 ? 0 : at(i-1));
		char_type next = (i == size() ? 0 : at(i));

		//character in position i-1 equals c?
		bool prev_equals_c = (i == 0 ? false : prev == c);

		//character in position i equals c?
		bool next_equals_c = (i == size() ? false : next == c);

		//CASE #2: c touches a c-run

		if(prev_equals_c or next_equals_c){

			//since position i touches a c-run, this vector can not be empty
			assert(runs_per_letter[c].size() > 0);

			//c-run that is extended with a new c
			ulint extended_run = ( prev_equals_c ? runs.rank1(i-1) : runs.rank1(i) );

			//extend run: insert k zeros in runs
			//if at(i-1) == c and at(i) != c, then insert 0s at position i-1
			//because position i-1 contains a 1

			assert( not (prev_equals_c and not next_equals_c) || runs[i-1] );
			runs.insert0( prev_equals_c and not next_equals_c ? i-1 : i, k );

			//here c must be present inside run_heads_ since
			//the new c touches a c-run
			assert(run_heads_.char_exists(c));

			//the extended run is the number 'extended_c_run' among all c-runs
			ulint extended_c_run = run_heads_.rank(extended_run,c);

			assert(extended_c_run < runs_per_letter[c].rank1());
			runs_per_letter[c].insert0(runs_per_letter[c].select1(extended_c_run), k);

			//n++;
			//R does not increase because c touches a c-run

			assert(runs_per_letter[c].size() > 0);

			assert( run_at( runs.rank1(i) ) == run_at( run_heads_.rank(runs.rank1(i),c), c ) );
			return;

		}

		//case #3: c does not touch c-runs

		//CASE #3.1: insertion at the beginning
		if(i==0){

			//next character exists and is different than c
			assert(next != c);

			runs.insert1(0);
			runs.insert0(0,k-1);

			run_heads_.insert(0,c);
			runs_per_letter[c].insert1(0);
			runs_per_letter[c].insert0(0,k-1);

			//n++;
			//R++;

			assert(runs_per_letter[c].size() > 0);

			assert( run_at( runs.rank1(i) ) == run_at( run_heads_.rank(runs.rank1(i),c), c ) );
			return;

		}

		//CASE #3.2: insertion at the end
		if(i==size()){

			assert(i>0);

			//previous character exists and is different than c
			assert(prev != c);

			runs.insert0(runs.size(),k-1);
			runs.insert1(runs.size());

			run_heads_.insert(number_of_runs(),c);

			runs_per_letter[c].insert0(runs_per_letter[c].size(), k-1);
			runs_per_letter[c].insert1(runs_per_letter[c].size());

			//n++;
			//R++;

			assert(runs_per_letter[c].size() > 0);

			assert( run_at( runs.rank1(i) ) == run_at( run_heads_.rank(runs.rank1(i),c), c ) );
			return;

		}

		//CASE #3.3: c falls between 2 runs of 2 characters different than c
		//example: aaaaaaaabbbbb -> aaaaaaaacbbbbb

		if(prev != next){

			assert(i>0);
			assert(i<size());
			assert(runs[i-1]);

			auto rk = runs.rank1(i);
			assert(number_of_runs()>1);
			assert(rk>0);
			assert(rk<=number_of_runs()-1);

			runs.insert1(i);
			runs.insert0(i,k-1);

			run_heads_.insert(rk,c);

			assert(run_heads_[rk-1]!=c and run_heads_[rk+1]!=c);

			ulint this_c_run = run_heads_.rank(rk,c);

			auto ins_pos = this_c_run == 0 ? 0 : runs_per_letter[c].select1(this_c_run-1)+1;

			runs_per_letter[c].insert1( ins_pos );
			runs_per_letter[c].insert0( ins_pos, k-1 );

			//n++;
			//R++;

			assert(runs_per_letter[c].size() > 0);

			assert( run_at( runs.rank1(i) ) == run_at( run_heads_.rank(runs.rank1(i),c), c ) );
			return;

		}

		//CASE #3.4: c falls inside a single a-run, where a != c

		assert(prev == next);
		assert(i>0);
		assert(i<size());
		assert(not runs[i-1]);

		//run that will be splitted
		ulint this_run = runs.rank1(i);

		//rank of the new c among all c-runs
		assert(this_run<=run_heads_.size());
		ulint this_c_run = run_heads_.rank(this_run,c);

		//rank of a among all a-runs
		//ulint this_a_run = run_heads_rank(this_run,prev);

		//this a will be the first of a new a-run, while previous a
		//will be last of a new a-run
		ulint a_rank = rank(i,prev);

		//runs[i-1] = true
		runs.set(i-1);

		//insert new c-run
		runs.insert1(i);
		runs.insert0(i,k-1);

		//split run
		run_heads_split(this_run,c);

		//insert a 0^k1 in c-runs
		auto ins_pos = this_c_run == 0 ? 0 : runs_per_letter[c].select1(this_c_run-1)+1;
		runs_per_letter[c].insert1( ins_pos	);
		runs_per_letter[c].insert0( ins_pos, k-1	);

		//insert a 1 in a-runs
		assert(a_rank>0);
		runs_per_letter[prev].set(a_rank-1);
		//runs_per_letter[prev].insert1(a_rank);

		//n++;
		//R += 2;

		assert(runs_per_letter[c].size() > 0);
		assert( run_at( runs.rank1(i) ) == run_at( run_heads_.rank(runs.rank1(i),c), c ) );

	}

	void push_back(char_type c){

		insert(size(),c);

	}

	void push_front(char_type c){

		insert(0,c);

	}

	//break range: given a range <l',r'> on the string and a character c, this function
	//breaks <l',r'> in maximal sub-ranges containing character c.
	//for simplicity and efficiency, we assume that characters at range extremities are both 'c'
	//thanks to the encoding (run-length), this function is quite efficient: O(|result|) ranks and selects
	/*vector<range_t> break_range(range_t rn,uchar c){

		auto l = rn.first;
		auto r = rn.second;

		assert(l<=r);
		assert(r<size());

		assert(operator[](l)==c);
		assert(operator[](r)==c);

		//retrieve runs that contain positions l and r
		auto run_l = run_of(l);
		auto run_r = run_of(r);

		//in this case rn contains only character c: do not break
		if(run_l.first==run_r.first) return {rn};

		vector<range_t> result;

		//first range: from l to the end of the run containing position l
		result.push_back({l,run_l.second});

		//rank of c's of interest in run_heads
		ulint rank_l = run_heads.rank(run_l.first,c);
		ulint rank_r = run_heads.rank(run_r.first,c);

		//now retrieve run bounds of all c-runs of interest
		for(ulint j = rank_l+1;j<rank_r;++j){

			result.push_back(run_range(run_heads.select(j,c)));

		}

		//now last (possibly incomplete) run

		auto range = run_range(run_heads.select(rank_r,c));
		result.push_back({range.first,r});

		return result;

	}*/

	ulint size(){return runs.size();}

	/*
	 * number of runs intersecting the interval range = [left,right).
	 * Note: right bound is excluded!
	 *
	 */
	ulint number_of_runs(pair<ulint,ulint> range){

		ulint l = range.first;
		ulint r = range.second;

		assert(l<=runs.size());
		assert(r<=runs.size());

		/*
		 * if interval not empty, add number of 1s in the interval and (not runs[r-1])
		 * (not runs[r-1]) = 1 iif last bit in the interval is 0 (i.e. if right bound falls inside a run)
		 */
		return	r<=l ? 0 :
				( runs.rank1(r) - runs.rank1(l) ) + (not runs[r-1]);

	}

	//total number of runs
	ulint number_of_runs(){return run_heads_.size();}

	//length of i-th run
	ulint run_at(ulint i){

		assert(i<number_of_runs());
		return (i==0) + runs.select1(i) - (i==0?0:runs.select1(i-1));

	}

	//length of i-th c-run
	ulint run_at(ulint i, char_type c){

		assert(i<runs_per_letter[c].rank1(runs_per_letter[c].size()));
		return (i==0) + runs_per_letter[c].select1(i) - (i==0?0:runs_per_letter[c].select1(i-1));

	}

	/*
	 * input: position on the string and character c
	 * output: number of c-runs before position i, excluded
	 * the run containing i (if it is a c-run)
	 */
	ulint run_rank(ulint i, char_type c){

		return run_heads_.rank(runs.rank1(i),c);

	}

	/*
	 * given a position i inside the string, return the interval [l,r) of the run containing i,
	 * i.e. i \in [l,r) (right position always exclusive)
	 */
	pair<ulint,ulint> locate_run(ulint i){

		assert(i<runs.size());

		//number of 1 before position i excluded
		ulint before = runs.rank(i);

		ulint l = before==0 ? 0 : runs.select(before-1)+1;

		ulint r = runs.select(before) + 1;

		return {l,r};

	}

	/*
	 * Total number of bits allocated in RAM for this structure
	 *
	 * WARNING: this measure is good only for relatively small alphabets (e.g. ASCII)
	 * as we use STL containers such as set and map which do not give direct info on
	 * the total memory allocated. The sizes of these containers are proportional
	 * to the alphabet size (but the constants involved are high since internally
	 * they can use heavy structures as RBT)
	 */
	ulint bit_size(){

		ulint size = sizeof(rle_string<sparse_bitvector_t,string_t>)*8;

		size += runs.bit_size();

		size += run_heads_.bit_size();

		for(auto e : runs_per_letter){

			size += (sizeof(e.first) + sizeof(e.second))*8;
			size += e.second.bit_size();

		}

		return size;

	}

	ulint serialize(ostream &out){

		ulint w_bytes=0;

		w_bytes += runs.serialize(out);
		w_bytes += run_heads_.serialize(out);

		ulint rpl_size = runs_per_letter.size();

		out.write((char*)&rpl_size, sizeof(rpl_size));
		w_bytes += sizeof(rpl_size);

		for(auto &e : runs_per_letter){

			out.write((char*)&e.first,sizeof(e.first));
			w_bytes += sizeof(e.first);

			w_bytes += e.second.serialize(out);

		}

		return w_bytes;

	}

	void load(istream &in){

		runs.load(in);
		run_heads_.load(in);

		ulint rpl_size;

		in.read((char*)&rpl_size, sizeof(rpl_size));

		for(ulint i=0;i<rpl_size;++i){

			char_type key;
			in.read((char*)&key,sizeof(key));

			runs_per_letter[key].load(in);

		}

	}

private:


	/*
	 * split i-th run head: a -> aca
	 */
	void run_heads_split(ulint i, char_type c){

		assert(i < number_of_runs());

		char_type r = run_heads_[i];

		assert(r!=c);

		run_heads_.insert(i+1,r);
		run_heads_.insert(i+1,c);

	}

	//main bitvector storing all run lengths. R bits set
	//a run of length n+1 is stored as 0^n1
	sparse_bitvector_t runs;

	//for each letter, its runs stored contiguously
	map<char_type,sparse_bitvector_t> runs_per_letter;

	//store run heads in a compressed string supporting access/rank/select/insert
	string_t run_heads_;

};

}

#endif /* INCLUDE_INTERNAL_RLE_STRING_HPP_ */
