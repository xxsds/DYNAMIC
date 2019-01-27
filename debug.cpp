// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

/*
 * debug.cpp
 *
 *  Created on: Oct 15, 2015
 *      Author: nico
 */

#include "spsi.hpp"
#include "spsi_check.hpp"
#include <chrono>
#include <dynamic.hpp>
#include <alphabet_encoder.hpp>
#include "include/algorithms/rle_lz77_v1.hpp"
#include "include/algorithms/rle_lz77_v2.hpp"

#include "include/internal/packed_vector.hpp"
#include "include/internal/wt_string.hpp"

using namespace std;
using namespace dyn;


ulint rank_vec(vector<ulint>& s, ulint i, ulint c){

	ulint r=0;

	for(ulint j=0;j<i;++j) r += s[j]==c;


	return r;

}

ulint select_vec(vector<ulint>& s, ulint i, ulint c){

	ulint r=0;

	for(ulint j=0;j<s.size();++j){

		if(s[j]==c and r == i) return j;

		r += s[j]==c;

	}

	assert(false);
	return 0;

}

void test_spsi(uint64_t n){

	uint32_t sigma = 300;

	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;

	auto t1 = high_resolution_clock::now();

	packed_spsi sp;
	spsi_check<> spc;

	srand(time(NULL));

	for(uint32_t i=0;i<n;++i){

		uint64_t x = rand()%sigma;
		uint64_t j = rand()%(spc.size()+1);

		spc.insert(j,x);
		sp.insert(j,x);

		if(spc.size()!=sp.size()){
			cout << spc.size() << "/" << sp.size()<<endl;
		}

		assert(sp.size()==i+1);
		assert(spc.size()==sp.size());

	}

	assert(sp.psum()==spc.psum());

	auto ps = sp.psum();

	cout << "testing access ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		assert(sp[i]==spc.at(i));

	}
	cout << " ok." << endl;

	cout << "testing search ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		uint64_t r = rand()%(1+sp.psum());

		assert(sp.search(r)==spc.search(r));

	}
	cout << " ok." << endl;

	cout << "testing search_r ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		uint64_t r = rand()%(1+sp.psum()+sp.size());

		assert(sp.search_r(r)==spc.search_r(r));

	}
	cout << " ok." << endl;

	cout << "testing contains ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		uint64_t r = rand()%(1+sp.psum());

		//if(sp.contains(r)!=spc.contains(r)) cout << r << " " << sp.contains(r) << " " << spc.contains(r) << endl;

		assert(sp.contains(r)==spc.contains(r));

	}
	cout << " ok." << endl;

	cout << "testing contains_r ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		uint64_t r = rand()%(1+sp.psum()+sp.size());

		//if(sp.contains_r(r)!=spc.contains_r(r)) cout << r << " " << sp.contains_r(r) << " " << spc.contains_r(r) << endl;

	}
	cout << " ok." << endl;

	cout << "testing psum ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		assert(sp.psum(i)==spc.psum(i));

	}
	cout << " ok." << endl;

	cout << "testing increment ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		uint64_t d = rand()%5;
		uint64_t j = rand()%spc.size();

		sp[j] += d;
		spc.increment(j,d);

	}
	cout << " ok." << endl;
	cout << "testing ++ ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		uint64_t j = rand()%spc.size();

		sp[j] ++;
		spc.increment(j,1);

	}
	cout << " ok." << endl;
	cout << "testing access after increment ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		assert(sp[i]==spc.at(i));

	}
	cout << " ok." << endl;

	cout << "testing decrement ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		assert(spc.size()==sp.size());

		uint64_t j = rand()%spc.size();

		auto val = sp.at(j);

		if(val>1){

			uint64_t d = (rand()%(val-1))+1;

			sp[j] -= d;
			spc.increment(j,d,true);

			assert(sp[j]==spc.at(j));

		}

	}
	cout << " ok." << endl;
	cout << "testing access after decrement ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		assert(sp[i]==spc.at(i));

	}
	cout << " ok." << endl;

	cout << endl << "ALLRIGHT!" << endl;
	cout << "bitsize = " << sp.bit_size() << endl;
	cout << "bits per integer = " << (double)sp.bit_size()/sp.size() << endl;

	auto t2 = high_resolution_clock::now();
	uint64_t total = duration_cast<duration<double, std::ratio<1>>>(t2 - t1).count();

	cout << total << " seconds." << endl;

	cout << "Memory freed successfully." << endl;

}


void benchmark_spsi(uint64_t size){

	uint32_t sigma = 256;

	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;

	packed_spsi sp;
	//gamma_spsi sp;

	srand(time(NULL));

	auto t1 = high_resolution_clock::now();

	cout << "benchmarking insert ..." << flush;
	for(uint32_t i=0;i<size;++i){

		uint64_t x = rand()%sigma;
		uint64_t j = rand()%(sp.size()+1);
		sp.insert(j,x);

		assert(sp.size()==i+1);

	}
	cout << " done." << endl;

	auto t2 = high_resolution_clock::now();

	cout << "benchmarking access ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		sp.at(rand()%size);

	}
	cout << " done." << endl;

	auto t3 = high_resolution_clock::now();

	cout << "benchmarking psum ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		sp.psum(rand()%size);

	}
	cout << " done." << endl;

	auto t4 = high_resolution_clock::now();

	cout << "benchmarking increment ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		sp.increment(rand()%size,1);

	}
	cout << " done." << endl;

	auto t5 = high_resolution_clock::now();

	cout << "benchmarking search ..." << flush;
	uint64_t pss = sp.psum();

	for(uint32_t i = 0;i<sp.size();++i){

		sp.search(rand()%pss);

	}
	cout << " done." << endl;

	auto t6 = high_resolution_clock::now();


	cout << "benchmarking contains ..." << flush;

	for(uint32_t i = 0;i<sp.size();++i){

		sp.contains(rand()%pss);

	}
	cout << " done." << endl;

	auto t7 = high_resolution_clock::now();

	cout << "benchmarking set ..." << flush;
	for(uint32_t i=0;i<size;++i){

		uint64_t x = rand()%sigma;
		uint64_t j = rand()%sp.size();
		sp.set(j,x);

	}
	cout << " done." << endl;

	auto t8 = high_resolution_clock::now();


	uint64_t sec_insert = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	uint64_t sec_access = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
	uint64_t sec_psum = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
	uint64_t sec_incr = std::chrono::duration_cast<std::chrono::microseconds>(t5 - t4).count();
	uint64_t sec_search = std::chrono::duration_cast<std::chrono::microseconds>(t6 - t5).count();
	uint64_t sec_cont = std::chrono::duration_cast<std::chrono::microseconds>(t7 - t6).count();
	uint64_t sec_set = std::chrono::duration_cast<std::chrono::microseconds>(t8 - t7).count();

	cout << endl << "ALLRIGHT! statistics: " << endl << endl;
	cout << "total bitsize = " << sp.bit_size() << endl;
	cout << "bits per integer = " << (double)sp.bit_size()/sp.size() << endl<<endl;

	cout << (double)sec_insert/sp.size() << " microseconds/insert" << endl;
	cout << (double)sec_access/sp.size() << " microseconds/access" << endl;
	cout << (double)sec_psum/sp.size() << " microseconds/psum" << endl;
	cout << (double)sec_incr/sp.size() << " microseconds/increment" << endl;
	cout << (double)sec_search/sp.size() << " microseconds/search" << endl;
	cout << (double)sec_cont/sp.size() << " microseconds/contains" << endl;
	cout << (double)sec_set/sp.size() << " microseconds/set" << endl;

}

void test_lciv(uint64_t n){

	uint32_t sigma = 300;

	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;

	auto t1 = high_resolution_clock::now();

	packed_lciv sp;
	spsi_check<> spc;

	srand(time(NULL));

	for(uint32_t i=0;i<n;++i){

		uint64_t x = rand()%sigma;
		uint64_t j = rand()%(spc.size()+1);

		spc.insert(j,x);
		sp.insert(j,x);

		if(spc.size()!=sp.size()){
			cout << spc.size() << "/" << sp.size()<<endl;
		}

		assert(sp.size()==i+1);
		assert(spc.size()==sp.size());

	}

	cout << "testing access ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		assert(sp[i]==spc.at(i));

	}

	cout << " ok." << endl;

	cout << "testing increment ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		uint64_t d = rand()%5;
		uint64_t j = rand()%spc.size();

		sp[j] += d;
		spc.increment(j,d);

	}
	cout << " ok." << endl;
	cout << "testing ++ ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		uint64_t j = rand()%spc.size();

		sp[j] ++;
		spc.increment(j,1);

	}
	cout << " ok." << endl;
	cout << "testing access after increment ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		assert(sp[i]==spc.at(i));

	}
	cout << " ok." << endl;

	cout << "testing decrement ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		assert(spc.size()==sp.size());

		uint64_t j = rand()%spc.size();

		auto val = sp.at(j);

		if(val>1){

			uint64_t d = (rand()%(val-1))+1;

			sp[j] -= d;
			spc.increment(j,d,true);

			assert(sp[j]==spc.at(j));

		}

	}
	cout << " ok." << endl;
	cout << "testing access after decrement ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		assert(sp[i]==spc.at(i));

	}
	cout << " ok." << endl;

	cout << endl << "ALLRIGHT!" << endl;
	cout << "bitsize = " << sp.bit_size() << endl;
	cout << "bits per integer = " << (double)sp.bit_size()/sp.size() << endl;

	auto t2 = high_resolution_clock::now();
	uint64_t total = duration_cast<duration<double, std::ratio<1>>>(t2 - t1).count();

	cout << total << " seconds." << endl;

	cout << "Memory freed successfully." << endl;

}


void benchmark_lciv(uint64_t size){

	uint32_t sigma = 256;

	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;

	packed_lciv sp;

	srand(time(NULL));

	auto t1 = high_resolution_clock::now();

	cout << "benchmarking insert ..." << flush;
	for(uint32_t i=0;i<size;++i){

		uint64_t x = rand()%sigma;
		uint64_t j = rand()%(sp.size()+1);
		sp.insert(j,x);

		assert(sp.size()==i+1);

	}
	cout << " done." << endl;

	auto t2 = high_resolution_clock::now();

	cout << "benchmarking access ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		sp.at(rand()%size);

	}
	cout << " done." << endl;

	auto t3 = high_resolution_clock::now();

	cout << "benchmarking increment ..." << flush;
	for(uint32_t i = 0;i<sp.size();++i){

		sp.increment(rand()%size,1);

	}
	cout << " done." << endl;

	auto t4 = high_resolution_clock::now();

	cout << "benchmarking set ..." << flush;
	for(uint32_t i=0;i<size;++i){

		uint64_t x = rand()%sigma;
		uint64_t j = rand()%sp.size();
		sp.set(j,x);

	}
	cout << " done." << endl;

	auto t5 = high_resolution_clock::now();


	uint64_t sec_insert = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	uint64_t sec_access = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
	uint64_t sec_incr = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
	uint64_t sec_set = std::chrono::duration_cast<std::chrono::microseconds>(t5 - t4).count();

	cout << endl << "ALLRIGHT! statistics: " << endl << endl;
	cout << "total bitsize = " << sp.bit_size() << endl;
	cout << "bits per integer = " << (double)sp.bit_size()/sp.size() << endl<<endl;

	cout << (double)sec_insert/sp.size() << " microseconds/insert" << endl;
	cout << (double)sec_access/sp.size() << " microseconds/access" << endl;
	cout << (double)sec_incr/sp.size() << " microseconds/increment" << endl;
	cout << (double)sec_set/sp.size() << " microseconds/set" << endl;

}

//typedef lciv<packed_vector,256,16> packed_;

void compare_bitvectors(uint64_t size){

	suc_bv dbv;
	bv_check gbv;

	srand(time(NULL));

	cout << "test insert ... " << flush;
	for(uint64_t i=0;i<size;++i){

		uint64_t j = rand()%(dbv.size()+1);
		bool b = rand()%2;

		dbv.insert(j,b);
		gbv.insert(j,b);

		assert(dbv.size()==gbv.size());

	}
	assert(dbv.size()==size);
	assert(gbv.size()==size);

	cout << "ok!" << endl;

	cout << "test rank1 ... " << flush;

	for(uint64_t i=0;i<=size;++i){

		if(dbv.rank1(i)!=gbv.rank1(i)){

			cout << "ERROR:" << endl;

			cout << "rle check =  ";
			for(ulint i=0;i<dbv.size();++i) cout << dbv[i];cout << endl;

			cout << "bv  check =  ";
			for(ulint i=0;i<gbv.size();++i) cout << gbv[i];cout << endl;

			cout << "rle rank1 =  ";
			for(ulint i=0;i<dbv.size();++i) cout << dbv.rank1(i) << " ";cout << endl;

			cout << "bv  rank1 =  ";
			for(ulint i=0;i<gbv.size();++i) cout << gbv.rank1(i) << " ";cout << endl;

		}

		assert(dbv.rank1(i)==gbv.rank1(i));

	}
	cout << "ok!" << endl;

	uint64_t ps = dbv.rank1(dbv.size());

	assert(gbv.rank1(gbv.size())==ps);

	cout << "test access ... " << flush;
	for(uint64_t i=0;i<size;++i){

		assert(dbv[i]==gbv[i]);

	}
	cout << "ok!" << endl;

	cout << "test select0 ... " << flush;
	uint64_t nzero = dbv.rank0(dbv.size());

	assert(nzero==gbv.rank0(gbv.size()));

	for(uint64_t i=0;i<size;++i){

		uint64_t x = rand()%nzero;

		assert(dbv.select0(x)==gbv.select0(x));

	}
	cout << "ok!" << endl;

	cout << "test select1 ... " << flush;

	for(uint64_t i=0;i<size;++i){

		uint64_t x = rand()%ps;

		assert(dbv.select1(x)==gbv.select1(x));

	}
	cout << "ok!" << endl;

	cout << "test rank0 ... " << flush;

	for(uint64_t i=0;i<=size;++i){

		assert(dbv.rank0(i)==gbv.rank0(i));

	}
	cout << "ok!" << endl;

	cout << "\nALLRIGHT! " << endl;


}

//p=probability of nonzero element
template<class dyn_str_t>
void benchmark_dyn_str(uint64_t size, uint64_t sigma, double p = 0.5){

	dyn_str_t bv;

	bv=dyn_str_t();

	srand(time(NULL));

	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;

	auto t1 = high_resolution_clock::now();

	cout << "insert ... " << flush;
	for(uint64_t i=0;i<size;++i){

		//if(i%10000==0 and i>0) cout << endl << i << " characters processed ...";

		ulint c = double(rand())/RAND_MAX < p ? rand()%(sigma-1)+1 : 0;

		bv.insert(rand()%(bv.size()+1),c);

	}
	cout << "done." << endl;

	auto t2 = high_resolution_clock::now();

	cout << "access ... " << flush;
	for(uint64_t i=0;i<size;++i){

		bv[rand()%bv.size()];

	}
	cout << "done." << endl;

	auto t3 = high_resolution_clock::now();

	cout << "rank ... " << flush;
	for(uint64_t i=0;i<size;++i){

		bv.rank(rand()%(bv.size()+1),rand()%sigma);

	}
	cout << "done." << endl;

	auto t4 = high_resolution_clock::now();

	uint64_t c = rand()%sigma;
	uint64_t nr_c = bv.rank(bv.size(),c);

	cout << "select ... " << flush;
	for(uint64_t i=0;i<size;++i){

		//if(i%100==0 and i>0) cout << endl << i << " characters processed ...";

		bv.select(i%nr_c,c);

	}
	cout << "done." << endl;
	auto t5 = high_resolution_clock::now();

	uint64_t sec_insert = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	uint64_t sec_access = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
	uint64_t sec_rank = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
	uint64_t sec_sel = std::chrono::duration_cast<std::chrono::microseconds>(t5 - t4).count();

	cout << (double)sec_insert/bv.size() << " microseconds/insert" << endl;
	cout << (double)sec_access/bv.size() << " microseconds/access" << endl;
	cout << (double)sec_rank/bv.size() << " microseconds/rank" << endl;
	cout << (double)sec_sel/size << " microseconds/sel" << endl;

	cout << "Bit size of the structure: " << bv.bit_size() << endl;

}

void check_strings(ulint size, ulint sigma){

	wt_str s1;
	wtrle_str s2;
	//str_check s2;
	//rle_str_check s3;
	rle_str s3;

	vector<ulint> truth;

	srand(time(NULL));

	cout << "testing insert ... " << flush;
	for(ulint i=0;i<size;++i){

		assert(s1.size() == s2.size());
		assert(s2.size() == s3.size());

		ulint j = rand()%(s1.size()+1);
		ulint c = rand()%sigma;

		s1.insert( j,c );
		s2.insert( j,c );
		s3.insert( j,c );

		assert(s1.size() == s2.size());
		assert(s2.size() == s3.size());

	}
	cout << "done. " << endl;

	for(ulint i=0;i<size;++i) truth.push_back(s1[i]);

	assert(s2.size() == size);
	assert(s3.size() == size);

	cout << "testing access ... " << flush;
	for(ulint i=0;i<size;++i){

		assert(s1[i] == s2[i]);
		assert(s2[i] == s3[i]);

	}
	cout << "done. " << endl;

	//assert(s3.check_consistency());

	cout << "testing rank ... " << flush;
	ulint c = s1[rand()%s1.size()];
	for(ulint i=0;i<=size;++i){

		assert(s1.char_exists(c));
		assert(s2.char_exists(c));
		assert(s3.char_exists(c));

		auto rr = rank_vec(truth,i,c);

		assert(rr == s1.rank(i,c));
		assert(rr == s2.rank(i,c));
		assert(rr == s3.rank(i,c));

	}
	cout << "done. " << endl;

	cout << "testing select ... " << flush;
	c = s1[rand()%s1.size()];
	for(ulint i=0;i<s1.rank(s1.size(),c);++i){

		auto ss = select_vec(truth, i,c);

		assert( ss == s1.select(i,c));
		assert( ss == s2.select(i,c));
		assert( ss == s3.select(i,c));

	}
	cout << "done. " << endl;

	cout << "testing rank(select) ... " << flush;
	c = s1[rand()%s1.size()];
	for(ulint i=0;i<s1.rank(s1.size(),c);++i){

		assert( s1.rank(s1.select(i,c),c ) == i);
		assert( s2.rank(s2.select(i,c),c ) == i);
		assert( s3.rank(s3.select(i,c),c ) == i);

	}
	cout << "done. " << endl;

	cout << "ALLRIGHT! " << endl;


}

void test_strings(ulint n, double P = 0.5){

	ulint sigma = 26;

	cout << endl << " *** gap_bv:" << endl;
	benchmark_dyn_str<gap_bv>(n, 2,P);

	cout << endl << " *** suc_bv:" << endl;
	benchmark_dyn_str<suc_bv>(n, 2,P);

	//cout << endl << " *** com_str:" << endl;
	//benchmark_dyn_str<wt_str>(n, sigma);

	//cout << endl << " *** rle_str:" << endl;
	//benchmark_dyn_str<rle_str>(n, sigma);

	//cout << endl << " *** wtrle_str:" << endl;
	//benchmark_dyn_str<wtrle_str>(n, sigma);

	//cout << endl << " *** wtgap_str:" << endl;
	//benchmark_dyn_str<wtgap_str>(n, sigma);

}

void test_bwt(){

	rle_bwt rlbwt;

	rlbwt = rle_bwt();

	string m = "mississdqdqdsajdnisjndiqncqncncoqsnippi";

	for(ulint i=0;i<m.size();++i)
		rlbwt.extend(m[m.size()-i-1]);

	for(ulint i=0;i<rlbwt.bwt_length();++i)
		cout << uchar(rlbwt[i]==rlbwt.get_terminator()?'#':rlbwt[i]);

	cout << endl;

	ulint j = rlbwt.get_terminator_position();
	for(ulint k=0;k<rlbwt.bwt_length();++k){

		cout << uchar(rlbwt[j]==rlbwt.get_terminator()?'#':rlbwt[j]);
		j = rlbwt.LF(j);

	}

	cout << endl;

	j = 0;
	for(ulint k=0;k<rlbwt.bwt_length();++k){

		cout << uchar(rlbwt[j]==rlbwt.get_terminator()?'#':rlbwt[j]);
		j = rlbwt.FL(j);

	}

	cout << endl;

	auto r = rlbwt.get_full_interval();
	r = rlbwt.LF(r,'i');
	r = rlbwt.LF(r,'s');
	r = rlbwt.LF(r,'s');

	cout << r.first << ", " << r.second<<endl;

}

template<class lz77_t>
void test_lz77(string in, string out){

	lz77_t lz77;

	{

		cout << "Detecting alphabet ... " << flush;
		std::ifstream ifs(in);

		lz77 = lz77_t(ifs);
		ifs.close();

		cout << "done." << endl;

	}

	std::ifstream ifs(in);
	std::ofstream os(out);

	lz77.parse(ifs,os,true);

	ifs.close();
	os.close();

}

template<class fmi_t>
int test_fmi(ulint n, ulint sigma, ulint n_str = 100, ulint str_len = 5) {

	string filename("/home/nico/fmi_test");

	fmi_t sp(sigma);

	cout << "populating ... " << flush;

	srand(time(NULL));

	vector<ulint> T;

	{
		vector<ulint> rev_text;

		for(ulint i=0;i<n;++i){

			ulint c = rand()%sigma;

			rev_text.push_back(c);
			sp.extend(c);

		}

		for(ulint i=0;i<n;++i) T.push_back(rev_text[n-i-1]);

	}
	cout << "done" << endl;

	ofstream ofs(filename);

	cout << "serializing ... " << endl;

	cout << "Written " << sp.serialize(ofs) << " Bytes." << endl;

	cout << "done" << endl;

	ofs.close();


	fmi_t sp2;

	ifstream ifs(filename);

	cout << "loading ... " << flush;

	sp2.load(ifs);

	cout << "done" << endl;

	ifs.close();

	cout << "testing equality ... " << flush;
	for(ulint i=0;i<sp.size();++i){

		if(sp[i]!=sp2[i]){
			cout << "error in extract.\n";
			exit(0);
		}

	}
	cout << "done" << endl;

	cout << "testing locate ... " << endl;
	for(ulint i=0;i<n_str;++i){

		ulint j = rand() % (sp.size() - str_len + 1 );

		vector<ulint> P;

		//extract pattern from text
		for(ulint k=j;k<j+str_len;++k) P.push_back(T[k]);

		auto o1 = sp.locate(P);
		auto o2 = sp2.locate(P);

		cout << "Checking " << o1.size() << " occurrences ... " << flush;
		if(o1!=o2){
			cout << "error in locate.\n";
			exit(0);
		}
		cout << "ok" << endl;

	}
	cout << "done" << endl;


}

int main(int argc,char** argv) {

	//test_fmi<wt_fmi>(203010, 17, 15, 3);

	rle_bwt bwt;

	string s = "AAACCCCCC$CCCDDDDDDD";

	bwt.build_from_string(s,'$');

	cout << "BWT built." << endl;

	for(ulint i=0;i<bwt.size();++i){

		ulint b = bwt[i];
		ulint t =  bwt.get_terminator();

		auto c = b==t ? '$' : (char)bwt[i];

		cout << c;

	}

	cout << endl;

	test_spsi( 100000 );

    //test_lciv( 10000 );
    //benchmark_spsi( 10000 );
    //benchmark_lciv( 10000 );
}


