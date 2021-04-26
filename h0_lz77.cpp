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
#include "dynamic/dynamic.hpp"
#include "dynamic/algorithms/h0_lz77.hpp"

using namespace std;
using namespace dyn;

ulint sa_rate = 0;
bool int_file = false;

void help(){

cout << "Build LZ77 using a zero-order compressed FM index." << endl << endl;
		cout << "Usage: h0_lz77 [options] <input_file> <output_file> " << endl;
		cout << "Options: " << endl;
		cout << "-s <sample_rate>   store one SA sample every sample_rate positions. default: 256." << endl;
		cout << "-i                 Interpret the file as a stream of 32-bits integers." << endl;
		cout << "input_file: file to be parsed" << endl;
		cout << "output_file: LZ77 triples <start,length,trailing_character> will be saved in binary format in this file" << endl << endl;
		cout << "Note: the file should terminate with a character (or int if -i) not appearing elsewhere." << endl;

		exit(0);

}


void parse_args(char** argv, int argc, int &ptr){

	assert(ptr<argc);

	string s(argv[ptr]);
	ptr++;

	if(s.compare("-s")==0){

		sa_rate = atoi(argv[ptr++]);

	}else if(s.compare("-i")==0){

		int_file = true;

	}else{
		cout << "Error: unrecognized '" << s << "' option." << endl;
		help();
	}

}


int main(int argc,char** argv) {

	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;

	if(argc < 3) help();

	//parse options

	int ptr = 1;

	if(argc<3) help();

	while(ptr<argc-2)
		parse_args(argv, argc, ptr);

	string in = string(argv[ptr++]);
	string out = string(argv[ptr]);

	using lz77_t = h0_lz77<wt_fmi>;

	lz77_t lz77;
	ulint DEFAULT_SA_RATE = lz77_t::DEFAULT_SA_RATE;

	sa_rate = not sa_rate ? DEFAULT_SA_RATE : sa_rate;

	auto t1 = high_resolution_clock::now();


	cout << "Sample rate is " << sa_rate << endl;

	if(not int_file){

		{
			cout << "Detecting alphabet ... " << flush;
			std::ifstream ifs(in);

			lz77 = lz77_t(ifs, sa_rate);

			cout << "done." << endl;
		}

		std::ifstream ifs(in);
		std::ofstream os(out, ios::binary);

		lz77.parse(ifs,os,1,true);

	}else{

		lz77 = lz77_t(~uint(0), sa_rate);
		std::ifstream ifs(in, ios::binary);
		std::ofstream os(out, ios::binary);

		lz77.parse_int(ifs,os,1,true);

	}

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


