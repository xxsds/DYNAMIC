// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.


/*
 * ContextAutomata.h
 *
 *  Created on: Jul 14, 2014
 *      Author: nicola
 */

#ifndef CONTEXTAUTOMATA_H_
#define CONTEXTAUTOMATA_H_

#include "includes.hpp"
#include "BackwardFileIterator.h"
#include "BackwardStringIterator.h"
#include "PartialSums.h"
#include "DynamicString.h"

namespace bwtil {

class ContextAutomata {

public:

	ContextAutomata(){};

	//ASSUMPTION: alphabet is {0,...,sigma-1}, where 0 is the terminator character (appearing only at the end of file)
	ContextAutomata(uint64_t k, BackwardIterator * bfr, bool verbose){

		init(bfr, verbose);
		build(k, bfr, verbose);

	}

	//overhead : k will be chosen such that the automaton will require approximately space n*overhead/100 bits, where n is the text length
	ContextAutomata(BackwardIterator * bfr, uint64_t overhead, bool verbose){
		init(bfr, verbose);

		//detect optimal k and build automaton

		if(verbose) cout << "\n Allowed memory overhead for the automaton = " << overhead << "%" << endl;
		if(verbose) cout << " Detecting optimal k ... " << endl;

		uint64_t _k = optimalK(overhead, bfr, verbose);

		if(verbose) cout << " Done. Optimal k = " << _k << endl;

		build( _k , bfr, verbose);

	}

	//default 5% of overhead
	ContextAutomata(BackwardIterator * bfr, bool verbose){

		init(bfr, verbose);

		//detect optimal k and build automaton
		//default 5% of overhead

		if(verbose) cout << "\n Allowed memory overhead for the automaton = 5%" << endl;
		if(verbose) cout << " Detecting optimal k ... " << endl;

		uint64_t _k = optimalK(5, bfr, verbose);

		if(verbose) cout << " Done. Optimal k = " << _k << endl;

		build( _k , bfr, verbose);

	}

	//jump from current state following the edge labeled with s. WARNING: alphabet must be {0,...,sigma-1}
	void goTo(symbol s){

		current_state = edge(current_state, s);

		if(current_state==null_ptr){
			cout << "ERROR (ContextAutomata) : using non-initialized edge.\n";
			exit(0);
		}

	};

	void goToASCII(symbol s){

		current_state = edge(current_state, ASCIItoCode(s) );

		if(current_state==null_ptr){
			cout << "ERROR (ContextAutomata) : using non-initialized edge.\n";
			exit(0);
		}

	};

	ulint currentState(){return current_state;};//return current state number
	ulint numberOfStates(){return number_of_k_mers;};

	void rewind(){current_state=0;};//return to initial state

	symbol ASCIItoCode(symbol c){return remapping[c];}
	symbol CodeToASCII(symbol c){return inverse_remapping[c];}

	symbol ASCIItoCodeNoTerminator(symbol c){if(c==0) return 0;  return remapping[c]-1;}
	symbol CodeToASCIINoTerminator(symbol c){return inverse_remapping[c+1];}

	uint64_t alphabetSize(){return sigma;}//included terminator

	ulint textLength(){return n;};

	uint64_t contextLength(){return k;}

private:

	//return true with probability p
	bool flip_coin(double p){

		return ((double)rand()/(double)RAND_MAX) <= p;

	}

	uint64_t optimalK(uint64_t overhead, BackwardIterator * bfr, bool verbose){

		//strategy: sample randomly n/log n characters of the text (in contiguous blocks of size B)
		//and try k=1,... until suitable k is found. There will be at most log_sigma n iterations, so work is linear.

		ulint B = 1000; //number of characters per block

		if(n<B)
			B = n;

		vector<symbol> sampled_text;

		srand(time(NULL));

		ulint nr_of_blocks = n/B;//total number of blocks in the text
		ulint sampled_n = n/(uint64_t)log2(n);//expected size of sampled text
		ulint nr_of_sampled_blocks = sampled_n/B;//expected number of sampled blocks
		double p = (double)nr_of_sampled_blocks/(double)nr_of_blocks;//probability that a block is sampled

		int perc,last_perc=-1;
		if(verbose)
			cout << "  Sampling text ... " << endl;

		ulint pos = n-1;//current position in the text
		while(not bfr->begin()){

			if(((n-1)-pos)%B==0){

				if(pos+1 >= B ){//if there are at least B characters to be sampled

					if((n-1)-pos==0 or flip_coin(p)){//randomly decide if sample the block

						for(uint64_t i=0;i<B;i++)	//sample B characters
							sampled_text.push_back( bfr->read() );

					}

				}

				perc = (100*((n-1)-pos))/n;

				if(perc>last_perc and (perc%5)==0 and verbose){
					cout << "  " << perc << "% done." << endl;
					last_perc=perc;
				}

			}

			bfr->read();//skip character on text
			pos--;

		}

		bfr->rewind();

		if(verbose)
			cout << "\n  Sampled text size = " << sampled_text.size() << endl;

		/*
		 * Extimate number of bits in memory for each context:
		 * O(sigma * log n) bits for each context.
		 *
		 * Structures in memory:
		 *
		 * vector<uint64_t > prefix_nr;
		 * vector<vector<uint64_t> > edges;
		 * CumulativeCounters * counters;
		 * DynamicString ** dynStrings;
		 * ulint * lengths;
		 *
		 */

		PartialSums sample_cumulative_counter = PartialSums(sigma,n);//sample of cumulative counter to extimate its memory consumption
		dynamic_string_t sample_dynstring;

		ulint bits_per_k_mer =
				8 * sizeof(dynamic_string_t *) + //pointers to DynamicStrings
				sample_cumulative_counter.bitSize()  + //cumulative counters
				sample_dynstring.bitSize() + //DynamicStrings
				8*sizeof(vector<uint64_t >) + //vector prefix_nr
				8*sizeof(uint64_t) + //content of vector prefix_nr
				8*sizeof(ulint); //lengths

		ulint bits_per_k_1_mer =
				8*sizeof(vector<uint64_t>) + //vector edges
				8*sigma*sizeof(uint64_t); //content of vector edges

		uint64_t log_n = log2(n+1);

		if(verbose) cout << "  Extimated number of bits per k-mer: " << bits_per_k_mer << endl;
		if(verbose) cout << "  Extimated number of bits per (k-1)-mer: " << bits_per_k_1_mer << endl;

		ulint nr_of_k_mers;
		ulint nr_of_k_1_mers=1;//number of (k-1)-mers

		// we want the highest k such that nr_of_contexts*bits_per_context <= n * (overhead/100)

		uint64_t _k = 1;//start from k=1.
		nr_of_k_mers=numberOfContexts( _k, sampled_text);

		if(verbose) cout << "  Number of " << _k << "-mers : " << nr_of_k_mers << endl;

		while( _k < log_n and (nr_of_k_mers * bits_per_k_mer + nr_of_k_1_mers * bits_per_k_1_mer <= (n * overhead)/100) ){

			_k++;

			nr_of_k_1_mers = nr_of_k_mers;
			nr_of_k_mers=numberOfContexts( _k, sampled_text);

			if(verbose) cout << "  Number of " << _k << "-mers : " << nr_of_k_mers << endl;

		}

		if(_k > 1)//we found the first _k above the threshold, so decrease _k.
			_k--;
		else
			_k = 1;//minimum k is 1

		return _k;

	}

	/*
	 * number of contexts of length k in the sampled text
	 */
	ulint numberOfContexts(uint64_t k, vector<symbol> sampled_text){

		sigma_pow_k_minus_one = 1;
		for(uint64_t i=0;i<k-1;i++)
			sigma_pow_k_minus_one *= sigma;

		ulint q = n/(log2(n)*log2(n)) + 1;//hash size: n/log^2 n

		vector<set<ulint> > H = vector<set<ulint> >( q,set<ulint>() );//the hash

		ulint context = (ulint)0;//first context

		H.at(context%q).insert(context);

		for(ulint i=0;i<sampled_text.size();i++){

			context = shift(context, ASCIItoCode(sampled_text.at(i)) );

			H.at(context%q).insert(context);

		}

		vector<ulint> k_mers;

		for(ulint i=0;i<q;i++)
			for (std::set<ulint>::iterator it=H.at(i).begin(); it!=H.at(i).end(); ++it)
				k_mers.push_back(*it);

		return k_mers.size();

	}

	void init(BackwardIterator * bfr, bool verbose){

		if(verbose) cout << "\n*** Building context automaton ***\n\n";

		bwIt = bfr;

		n = bwIt->length();
		null_ptr = ~((uint64_t)0);

		if(verbose) cout << " Text length is " << n << endl;

		remapping = vector<uint64_t>(256);
		inverse_remapping = vector<symbol>(256);

		for(uint64_t i=0;i<256;i++){
			remapping[i]=empty;
			inverse_remapping[i]=0;
		}

		if(verbose) cout << "\n scanning file to detect alphabet ... " << endl;

		vector<symbol> alphabet = vector<symbol>();

		ulint symbols_read=0;
		vector<bool> inserted = vector<bool>(256,false);

		int perc,last_perc=-1;

		while(not bwIt->begin()){

			symbol s = bwIt->read();

			if(s==0){

				cout << "ERROR while reading input text : the text contains a 0x0 byte.\n";
				exit(0);

			}

			if(not inserted.at(s)){
				inserted.at(s) = true;
				alphabet.push_back(s);
			}

			perc = (100*symbols_read)/n;

			if(perc>last_perc and (perc%5)==0 and verbose){
				cout << " " << perc << "% done." << endl;
				last_perc=perc;
			}

			symbols_read++;

		}

		if(verbose) cout << " done.\n\n Sorting alphabet ... " << flush;

		std::sort(alphabet.begin(),alphabet.end());

		if(verbose) cout << "done. Alphabet size: sigma = " << alphabet.size() << endl;

		sigma = 1;//code 0x0 is for the terminator

		//if(verbose) cout << "\n Alphabet = { ";

		for (uint64_t i=0;i<alphabet.size();i++){

			if(remapping[alphabet.at(i)]==empty){//new symbol

				remapping[alphabet.at(i)] = sigma;
				sigma++;

			}

			//if(verbose) cout << alphabet.at(i) << ' ';

		}

		//if(verbose) cout << "}\n\n";

		if(verbose) cout << "\n Alphabet (ASCII codes) = { ";

		for (uint64_t i=0;i<alphabet.size();i++)
			if(verbose) cout << (ulint)alphabet.at(i) << ' ';

		if(verbose) cout << "}" << endl;

		TERMINATOR = 0;

		for(uint64_t i=1;i<256;i++)
			if(remapping[i]!=empty)
				inverse_remapping[remapping[i]] = i;

		inverse_remapping[TERMINATOR] = 0;//0 is the terminator appended in the file

		bwIt->rewind();

	}

	void build(uint64_t k, BackwardIterator * bfr, bool verbose){

		this->k=k;
		this->bwIt = bfr;

		sigma_pow_k_minus_one = 1;
		for(uint64_t i=0;i<k-1;i++)
			sigma_pow_k_minus_one *= sigma;

		ulint q = n/(log2(n)*log2(n)) + 1;//hash size: n/log^2 n

		vector<set<ulint> > H = vector<set<ulint> >( q,set<ulint>() );//the hash

		if(verbose) cout << "\n detecting k-mers ... " << endl;

		ulint context = (ulint)0;//first context

		H.at(context%q).insert(context);

		int perc,last_perc=-1;
		ulint symbols_read=0;

		while(not bfr->begin()){

			context = shift(context, ASCIItoCode(bfr->read()) );

			H.at(context%q).insert(context);

			perc = (100*symbols_read)/n;

			if(perc>last_perc and (perc%5)==0 and verbose){
				cout << " " << perc << "% done." << endl;
				last_perc=perc;
			}

			symbols_read++;

		}

		bfr->rewind();

		if(verbose) cout << " done.\n\n sorting k-mers ... " << flush;

		vector<ulint> k_mers;

		for(ulint i=0;i<q;i++)
			for (std::set<ulint>::iterator it=H.at(i).begin(); it!=H.at(i).end(); ++it)
				k_mers.push_back(*it);

		std::sort(k_mers.begin(),k_mers.end());

		number_of_k_mers = k_mers.size();

		if(verbose) cout << "done. " << k_mers.size() << " nonempty contexts of length k = " << k << " (including contexts containing terminator character)"  << endl;

		if(verbose) cout << " building automaton edges ... "<< endl;

		uint64_t nr_of_prefixes=0;
		prefix_nr.push_back(nr_of_prefixes);

		for(uint64_t i=1;i<number_of_k_mers;i++){

			if( prefix(k_mers.at(i)) == prefix(k_mers.at(i-1)) )
				prefix_nr.push_back(nr_of_prefixes);
			else{

				nr_of_prefixes++;
				prefix_nr.push_back(nr_of_prefixes);

			}

		}

		nr_of_prefixes++;

		edges = vector<vector<uint64_t> >(nr_of_prefixes, vector<uint64_t>(sigma,null_ptr)  );

		context = (ulint)0;//first context
		current_state = 0;

		perc=0;
		last_perc=-1;
		symbols_read=0;

		ulint context_from;
		ulint context_to;

		for(ulint i = 0;i<number_of_k_mers;i++ ){//for each k-mer

			context_from = k_mers.at(i);

			for(symbol s = 0;s<sigma;s++){//for each symbol

				if( edge(i,s) == null_ptr ){//edge does not exist: create it

					context_to = shift(context_from, s );

					//search context position and store new pointer

					setEdge(i, s, searchContext(context_to, k_mers));

				}

			}

			perc = (100*i)/number_of_k_mers;
			if(verbose and perc>last_perc and perc%10==0){

				last_perc=perc;
				cout << " " << perc << "% Done." << endl;

			}

		}

		rewind();//go back to initial state

		if(verbose) cout << " done." << endl;
		if(verbose) cout << "\nContext automaton completed." << endl;

	}

	static const uint64_t empty = 256;

	symbol TERMINATOR=0;//0x0

	BackwardIterator * bwIt = NULL;

	vector<symbol> inverse_remapping;//from symbol -> to char (file)
	vector<uint64_t> remapping;//from char (file) -> to symbols in {0,...,sigma-1}

	ulint current_state = 0;
	uint64_t null_ptr = 0;

	vector<uint64_t > prefix_nr;//for each k_mer, address of its prefix in the array edges
	vector<vector<uint64_t> > edges;//sigma edges for each (k-1)-mer

	ulint prefix(ulint context){ return (context - (context%sigma))/sigma; }
	ulint shift(ulint context, symbol s){ return prefix(context) + ((ulint)s)*sigma_pow_k_minus_one;	}

	uint64_t edge(uint64_t state, symbol s){ return edges.at( prefix_nr.at(state) ).at(s); }
	void setEdge(uint64_t state, symbol s, uint64_t value){ edges.at( prefix_nr.at(state) ).at(s) = value; }
	uint64_t searchContext(ulint context, vector<ulint> k_mers){ return std::lower_bound(k_mers.begin(),k_mers.end(),context) - k_mers.begin(); }

	ulint number_of_k_mers = 0;

	uint64_t sigma = 0;//alphabet size
	uint64_t k = 0;//context length
	ulint sigma_pow_k_minus_one = 0;
	ulint n = 0;

};

} /* namespace bwtil */
#endif /* CONTEXTAUTOMATA_H_ */
