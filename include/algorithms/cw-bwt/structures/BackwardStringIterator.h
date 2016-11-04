/*
 *  This file is part of BWTIL.
 *  Copyright (c) by
 *  Nicola Prezza <nicolapr@gmail.com>
 *
 *   BWTIL is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   BWTIL is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details (<http://www.gnu.org/licenses/>).
 */

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
