// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

#include <chrono>

#include "dynamic/algorithms/cw_bwt.hpp"

using namespace bwtil;
using namespace std;

 int main(int argc,char** argv) {

	if(argc != 3 and argc != 4){
		cout << "*** context-wise BWT construction in compressed space ***\n";
		cout << "Usage: cw-bwt text_file bwt_file [k]\n";
		cout << "where:\n";
		cout << "- text_file is the input text file. Input file must not contain a 0x0 byte since the algorithm uses it as text terminator.\n";
		cout << "- bwt_file is the output bwt file. This output file will contain a 0x0 terminator and thus will be 1 byte longer than the input file.\n";
		cout << "- k (automatically detected if not specified) is the entropy order (context length).\n";
		cout << "WARNING: for high values of k, the memory requirements approach n log n. If you specify k, choose it carefully!\n";
		cout << "For more informations, read the file README.\n";
		exit(0);
	}

    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;

    auto t1 = high_resolution_clock::now();

	cw_bwt cwbwt;

	/*
	 * Note: in this example, the text is loaded from disk and the bwt is directly saved to disk. The total RAM occupancy is therefore COMPRESSED, i.e.
	 * comparable to the size of the compressed input text file.
	 *
	 * It is possible (see comments below) also to build the bwt from/to strings, but in this way also the input/output strings will be stored in memory,
	 * resulting in higher RAM requirements.
	 *
	 * If you want to keep RAM usage to a minimum, it is recommended that you proceed as follows:
	 *
	 * 1) save your text (string/array) to disk
	 * 2) free memory
	 * 3) run cw-bwt loading the text from the file created in step 1) (see below)
	 * 4) save directly the bwt to disk (see below)
	 * 5) free memory
	 * 6) load in RAM the bwt created in step 4)
	 *
	 */

	//build bwt from a text file:

	if(argc==3){//k autodetected
		string path(argv[1]);
		//cw_bwt::path means that the first argument has to be interpreted as a file path rather than a text string
		cwbwt = cw_bwt(path,cw_bwt::path,true);
	}
	if(argc==4){//the user has specified k
		string path(argv[1]);
		cwbwt = cw_bwt(path,cw_bwt::path,atoi(argv[3]),true);
	}
	/*
	 * If, instead, you want to compute the bwt of a string, create a cw_bwt object as follows:
	 *
	 *
	 * string str = "mississippi";
	 * cwbwt = cw_bwt(str,cw_bwt::text); // optimal k autodetected
	 *
	 * or
	 *
	 * cwbwt = cw_bwt(str,cw_bwt::text, your_k_value,true); // you choose k
	 *
	 * However, this requires more space in RAM since the input text string is kept in memory together with the structures of cwbwt
	 *
	 */

	//save to file the bwt without occupying additional RAM
	cwbwt.toFile(argv[2]);

	/*
	 * If, instead, you want a string object containing the bwt, call
	 *
	 * string bwt = cwbwt.toString();
	 *
	 * However, this requires more space in RAM since the string bwt is kept in memory together with the structures of cwbwt
	 * WARNING: if you directly print cwbwt.toString(), you won't see the terminator character since it is a 0x0 byte.
	 *
	 */

	//printRSSstat(cwbwt.length());

	auto t2 = high_resolution_clock::now();
	ulint total = duration_cast<duration<double, std::ratio<1>>>(t2 - t1).count();

	if(total>=3600){

		uint h = total/3600;
		uint m = (total%3600)/60;
		uint s = (total%3600)%60;

		cout << "Total time: " << total << " seconds. ("<< h << "h " << m << "m " << s << "s" << ")"<<endl;

	}else if (total>=60){

		uint m = total/60;
		uint s = total%60;

		cout << "Total time: " << total << " seconds. ("<< m << "m " << s << "s" << ")"<<endl;

	}else{

		cout << "Total time: " << total << " seconds." << endl;

	}
 }



