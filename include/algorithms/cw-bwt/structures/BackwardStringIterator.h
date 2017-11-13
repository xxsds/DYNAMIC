// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

/*
 * BackwardArrayIterator.h
 *
 *  Created on: Aug 16, 2014
 *      Author: nicola
 */

#ifndef BACKWARDSTRINGITERATOR_H_
#define BACKWARDSTRINGITERATOR_H_

#include "includes.hpp"
#include "BackwardIterator.h"

namespace bwtil {

class BackwardStringIterator : public BackwardIterator{

	private:

	string in_str;

	ulint position = 0;//1 step ahead of next position to be read

public:

	BackwardStringIterator(){};

	BackwardStringIterator(string &in_str){

		this->in_str=in_str;
		position = in_str.size();

	}

	void rewind(){//go back to EOF

		position = in_str.size();

	}

	symbol read(){

		if(position>0){

			position--;
			return in_str.at(position);

		}

		return 0;

	}

	bool begin(){ return position==0; };//no more symbols to be read

	void close(){};//empty; nothing to be done with string

	ulint length(){return in_str.size(); };

};

}

#endif /* BACKWARDSTRINGITERATOR_H_ */
