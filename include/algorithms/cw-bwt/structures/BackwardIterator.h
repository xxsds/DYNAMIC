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
 * BackwardIterator.h
 *
 *  Created on: Aug 16, 2014
 *      Author: nicola
 */

#ifndef BACKWARDITERATOR_H_
#define BACKWARDITERATOR_H_

namespace bwtil {

class BackwardIterator{

public:

	BackwardIterator(){};

	virtual ~BackwardIterator(){};

	virtual void rewind(){};

	virtual symbol read(){return 0;};

	virtual bool begin(){return true;};

	virtual void close(){};

	virtual ulint length(){return 0;};

};

}

#endif /* BACKWARDITERATOR_H_ */
