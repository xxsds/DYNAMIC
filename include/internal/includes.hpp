// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

/*
 * includes.hpp
 *
 *  Created on: Oct 15, 2015
 *      Author: nico
 */

#ifndef INCLUDES_HPP_
#define INCLUDES_HPP_


#include "stdint.h"
#include <string>
#include <iostream>
#include <set>
#include <vector>
#include <fstream>
#include <sstream>
#include <cassert>
#include <map>
#include <cmath>
#include <algorithm>

#define WORD_SIZE 64;

using namespace std;

typedef unsigned char uchar;
typedef uint64_t ulint;

typedef unsigned char symbol;
typedef pair<uint64_t,uint64_t> range_t;

/*
 * input: an input stream of characters
 * output: character frequencies
 */
inline vector<pair<ulint,double> > get_frequencies(istream& in){

	vector<double> freqs(256,0);

	char c;
	ulint size=0;
	while(in.get(c)){

		assert(uchar(c) < freqs.size());

		freqs[uchar(c)] += 1;
		size++;

	}

	for(auto &f : freqs) f /= size;

	vector<pair<ulint,double> > res;

	for(ulint i=0;i<256;++i){

		res.push_back({i,freqs[i]});

		//if(freqs[i]>0)	cout << uchar(i) << " -> " << freqs[i] << endl;

	}

	return res;

}

#endif /* INCLUDES_HPP_ */
