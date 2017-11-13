// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

/*
 * h0_lz77.cpp
 *
 *  Compute the LZ77 parsing in zero-order compressed space
 *  using a dynamic Huffman-encoded FM index with
 *  a sparse SA sampling (1 sample every 256 characters)
 *
 *  Space is ( n*H0 + n + (n/k)*log n )(1+o(1)) bits.
 *
 *  Type of input text here is uchar
 *
 *
 */

#include <chrono>
#include <dynamic.hpp>
#include <algorithms/h0_lz77.hpp>

using namespace std;
using namespace dyn;

int main(int argc,char** argv) {

	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;

	if(argc!=3 and argc !=4){

		cout << "Build LZ77 using a zero-order compressed FM index." << endl << endl;
		cout << "Usage: h0_lz77 [sample_rate] <input_file> <output_file> " << endl;
		cout << "   sample_rate: store one SA sample every sample_rate positions. default: 256." << endl;
		cout << "   input_file: file to be parsed" << endl;
		cout << "   output_file: LZ77 triples <start,length,char> will be saved in text format in this file" << endl;

		exit(0);

	}

	using lz77_t = h0_lz77<wt_fmi>;

	/*
	 * uncomment this (and comment the above line) to use instead a
	 * run-length encoded FM index.
	 */
	//using lz77_t = h0_lz77<rle_fmi>;

	auto t1 = high_resolution_clock::now();

	lz77_t lz77;
	ulint DEFAULT_SA_RATE = lz77_t::DEFAULT_SA_RATE;

	ulint sa_rate = argc == 3 ? DEFAULT_SA_RATE : atoi(argv[1]);

	sa_rate = sa_rate == 0 ? 1 : sa_rate;

	string in(argv[1+(argc==4)]);
	string out(argv[2+(argc==4)]);

	cout << "Sample rate is " << sa_rate << endl;

	{

		cout << "Detecting alphabet ... " << flush;
		std::ifstream ifs(in);

		lz77 = lz77_t(ifs, sa_rate);
		ifs.close();

		cout << "done." << endl;

	}

	std::ifstream ifs(in);
	std::ofstream os(out);

	lz77.parse(ifs,os,15,true);

	ifs.close();
	os.close();

	auto t2 = high_resolution_clock::now();

	uint64_t sec = std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count();

	ulint bitsize = lz77.bit_size();

	cout << endl << "done" << endl;
	cout << " Total time: " << (double)sec << " seconds" << endl;
	cout << " Size of the structures (bits): " << bitsize << endl;
	cout << " Size of the structures (Bytes): " << bitsize/8 << endl;
	cout << " Size of the structures (KB): " << (bitsize/8)/1024 << endl;
	cout << " Size of the structures (MB): " << ((bitsize/8)/1024)/1024 << endl;


}


