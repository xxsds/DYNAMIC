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
#include "packed_block.hpp"
#include <alphabet_encoder.hpp>

using namespace std;
using namespace dyn;

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

		assert(sp.contains_r(r)==spc.contains_r(r));

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

	uint32_t sigma = 300;

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

void compare_bitvectors(uint64_t size){

	dyn_bv dbv;
	gap_bv gbv;

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

	cout << "test rank1 ... " << flush;

	for(uint64_t i=0;i<=size;++i){

		assert(dbv.rank1(i)==gbv.rank1(i));

	}
	cout << "ok!" << endl;

	cout << "\nALLRIGHT! " << endl;


}

template<class bv_type>
void benchmark_bitvector(uint64_t size){

	bv_type bv;

	srand(time(NULL));

	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;

	auto t1 = high_resolution_clock::now();

	cout << "insert ... " << flush;
	for(uint64_t i=0;i<size;++i){

		bv.insert(rand()%(bv.size()+1),rand()%2);

	}
	cout << "done." << endl;

	auto t2 = high_resolution_clock::now();

	cout << "access ... " << flush;
	for(uint64_t i=0;i<size;++i){

		bv[rand()%bv.size()];

	}
	cout << "done." << endl;

	auto t3 = high_resolution_clock::now();

	cout << "rank1 ... " << flush;
	for(uint64_t i=0;i<size;++i){

		bv.rank1(rand()%(bv.size()+1));

	}
	cout << "done." << endl;

	auto t4 = high_resolution_clock::now();

	cout << "rank0 ... " << flush;
	for(uint64_t i=0;i<size;++i){

		bv.rank0(rand()%(bv.size()+1));

	}
	cout << "done." << endl;

	auto t5 = high_resolution_clock::now();

	uint64_t nr1 = bv.rank1(bv.size());

	cout << "select1 ... " << flush;
	for(uint64_t i=0;i<size;++i){

		bv.select1(rand()%nr1);

	}
	cout << "done." << endl;

	uint64_t nr0 = bv.rank0(bv.size());

	auto t6 = high_resolution_clock::now();

	cout << "select0... " << flush;
	for(uint64_t i=0;i<size;++i){

		bv.select0(rand()%nr0);

	}
	cout << "done." << endl;

	auto t7 = high_resolution_clock::now();

	uint64_t sec_insert = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	uint64_t sec_access = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
	uint64_t sec_rank1 = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
	uint64_t sec_rank0 = std::chrono::duration_cast<std::chrono::microseconds>(t5 - t4).count();
	uint64_t sec_sel1 = std::chrono::duration_cast<std::chrono::microseconds>(t6 - t5).count();
	uint64_t sec_sel0 = std::chrono::duration_cast<std::chrono::microseconds>(t7 - t6).count();

	cout << (double)sec_insert/bv.size() << " microseconds/insert" << endl;
	cout << (double)sec_access/bv.size() << " microseconds/access" << endl;
	cout << (double)sec_rank1/bv.size() << " microseconds/rank1" << endl;
	cout << (double)sec_rank0/bv.size() << " microseconds/rank0" << endl;
	cout << (double)sec_sel1/bv.size() << " microseconds/sel1" << endl;
	cout << (double)sec_sel0/bv.size() << " microseconds/sel0" << endl;

	auto bs = bv.bit_size();
	cout << "Bit size of the structure = " << bs << " (" << (double)bs/size << " n bits)" << endl;

}

int main(int argc,char** argv) {

	//compare_bitvectors(100000);

	//cout << "GAP BITVECTOR: " << endl;
	//benchmark_bitvector<gap_bv>(10000000);

	//benchmark_spsi(11600000);

	cout << "\nSUCCINCT BITVECTOR: " << endl;
	benchmark_bitvector<dyn_bv>(10000000);

}
