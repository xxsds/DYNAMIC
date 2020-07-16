// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

/*
 * rle_lz77_v1.cpp
 *
 *  Compute the LZ77 parsing in run-compressed space
 *  using a dynamic run-length encoded BWT with
 *  a sparse SA sampling (2 samples per BWT run).
 *
 *  Space is O(R log n) bits. Be aware however that the constant hidden in
 *  the big-O notation is quite high: space is around 6Rlog n bits.
 *  See the algorithm rle_lz77_v2 for a more space-efficient parse.
 *
 *  Type of input text here is uchar
 *
 *  From the paper: Alberto Policriti and Nicola Prezza, "Computing LZ77 in Run-Compressed Space"
 *
 *
 */

#include <chrono>
#include "dynamic/dynamic.hpp"
#include "dynamic/algorithms/rle_lz77_v1.hpp"

using namespace std;
using namespace dyn;

int main(int argc,char** argv) {

	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;

	if(argc!=3){

		cout << "Build LZ77 using a run-length encoded BWT with sparse SA sampling (2 samples per BWT run)." << endl << endl;
		cout << "Usage: rle_lz7_v1 <input_file> <output_file> " << endl;
		cout << "   input_file: file to be parsed" << endl;
		cout << "   output_file: LZ77 triples <start,length,char> will be saved in text format in this file" << endl;

		exit(0);

	}

	using lz77_t = rle_lz77_v1;

	auto t1 = high_resolution_clock::now();

	lz77_t lz77;
	string in(argv[1]);
	string out(argv[2]);

	{

		cout << "Detecting alphabet ... " << flush;
		std::ifstream ifs(in);

		lz77 = lz77_t(ifs);
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


