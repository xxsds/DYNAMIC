// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

/*
 * rle_bwt.cpp
 *
 *  Compute the BWT of the reversed input file in run-compressed space
 *
 *  Space is 2(R log(n/R)+log sigma) bits.
 *
 */

#include <chrono>
#include "dynamic/dynamic.hpp"

using namespace std;
using namespace dyn;

int main(int argc,char** argv) {

	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;

	if(argc!=3){

		cout << "Build the BWT of the reversed text with a dynamic run-length encoded BWT structure" << endl << endl;
		cout << "Usage: rle_bwt <input_file> <output_file> " << endl;
		cout << "   input_file: compute BWT of the reverse of this file" << endl;
		cout << "   output_file: output BWT file" << endl;

		exit(0);

	}

	auto t1 = high_resolution_clock::now();

	string in(argv[1]);
	string out(argv[2]);

	rle_bwt bwt;

	{

		cout << "Detecting alphabet ... " << flush;
		std::ifstream ifs(in);

		auto freqs = get_frequencies(ifs);
		bwt = rle_bwt(freqs);

		ifs.close();

		cout << "done." << endl;

	}

	std::ifstream ifs(in);
	std::ofstream os(out);

	ulint j = 0;
	long int step = 1000000;	//print status every step characters
	long int last_step = 0;

	cout << "Building RLBWT ..." << endl;

	char c;
	while(ifs.get(c)){

		if(j>last_step+(step-1)){

			last_step = j;
			cout << " " << j << " characters processed ..." << endl;

		}

		bwt.extend( uchar(c) );

		j++;

	}

	for(ulint i=0;i<bwt.size();++i){

		auto cc = bwt[i];

		c = cc == bwt.get_terminator() ? 0 : char(cc);

		os.put(c);

	}

	auto t2 = high_resolution_clock::now();
	uint64_t sec = std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count();

	cout << "Number of runs in the BWT : " << bwt.number_of_runs() << endl;

	ifs.close();
	os.close();

	ulint bitsize = bwt.bit_size();

	cout << endl << "done" << endl;
	cout << " Total time: " << (double)sec << " seconds" << endl;
	cout << " Size of the structures (bits): " << bitsize << endl;
	cout << " Size of the structures (Bytes): " << bitsize/8 << endl;
	cout << " Size of the structures (KB): " << (bitsize/8)/1024 << endl;
	cout << " Size of the structures (MB): " << ((bitsize/8)/1024)/1024 << endl;

}


