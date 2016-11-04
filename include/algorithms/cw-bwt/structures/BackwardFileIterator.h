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
 * BackwardFileIterator.h
 *
 *  Created on: Jun 24, 2014
 *      Author: nicola
 *
 *      Description: this class offers a simple view to scan efficiently a file backwards.
 *      The backward read of the file is implemented buffering chunks of size n/(log^2(n)) from the end
 *      In this way, only log^2(n) calls to fseek are necessary and the total RAM occupancy is only n/(log^2(n))
 *
 *
 */

#ifndef BACKWARDFILEITERATOR_H_
#define BACKWARDFILEITERATOR_H_

#include "includes.hpp"
#include "BackwardIterator.h"

namespace bwtil {

class BackwardFileIterator  : public BackwardIterator{

public:

	BackwardFileIterator(string &path){

		this->path=path;

		fp = fopen(path.c_str(), "rb");

		if (fp == NULL){
		  cout << "Error while opening file " << path <<endl;
		  exit(0);
		}

		fseek(fp, 0, SEEK_END);
		n = ftell(fp);

		if (n == 0){
		  cout << "Error: file " << path << " has length 0." << endl;
		  exit(0);
		}

		bufferSize = n/(ulint)(log2(n+1)*log2(n+1));//read file in chunks of n/log^2 n bytes (in total log^2 n sequential reads of the file)
		if(bufferSize==0)
			bufferSize = 1;

		buffer = new symbol[bufferSize];

		/*cout << "bufferSize="<<bufferSize<<endl;
		cout << "txt len="<<n<<endl;*/

		rewind();

	}

	void rewind(){//go back to EOF

		offset = (n/bufferSize)*bufferSize;

		if(offset==n)
			offset = n - bufferSize;

		ulint size = n-offset;

		fseek ( fp , offset , SEEK_SET );

		if(fread(buffer, sizeof(symbol), size, fp)==0){
			cout << "Error while reading file " << path <<endl;
			exit(0);
		}

		begin_of_file=false;

		ptr_in_buffer = size-1;

	}

	symbol read(){

		symbol s = buffer[ptr_in_buffer];

		if(ptr_in_buffer==0){//read new chunk

			if(offset==0){
				begin_of_file=true;
				return s;
			}

			offset -= bufferSize;

			fseek ( fp , offset , SEEK_SET );

			if(fread(buffer, sizeof(symbol), bufferSize, fp)==0){
				cout << "Error while reading file " << path <<endl;
				exit(0);
			}

			ptr_in_buffer = bufferSize-1;

		}else{
			ptr_in_buffer--;
		}

		return s;

	}

	bool begin(){return begin_of_file;};//no more symbols to be read

	void close(){fclose(fp);delete [] buffer;};//close file

	ulint length(){return n;};

private:

	ulint n;
	ulint bufferSize;//n/(log^2(n))

	bool begin_of_file;//begin of file reached

	string path;

	symbol * buffer;

	ulint ptr_in_buffer;//pointer to the current position in buffer
	ulint offset;//offset (in the file) of the byte after the next chunk to be read

	FILE * fp;//pointer to file.


};

} /* namespace compressed_bwt_construction */
#endif /* BACKWARDFILEITERATOR_H_ */
