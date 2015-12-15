//============================================================================
// Name        : bwt.hpp
// Author      : Nicola Prezza
// Description : Dynamic (only append) compressed BWT.

/*
 * dynamic BWT, template on a dynamic string type (for the BWT) and on a RLE string type (for
 * the first column of the BWT matrix)
 * This class permits to extend the BWT by left-extending the text and do backward search.
 * Note that this class does not store a suffix array sampling: locate is not supported.
 *
 * Note: alphabet character 2^64-1 is reserved for the BWT terminator
 *
 */
//============================================================================

#ifndef DYNAMICBWT_H_
#define DYNAMICBWT_H_

#include <includes.hpp>

namespace dyn {

template <	class dynamic_string_type,	//to encode BWT
			class rle_string_type		//to encode column F
			>
class bwt {
public:

	//we allow any alphabet
	typedef uint64_t char_type;

	/*
	 * Constructor #1
	 *
	 * Alphabet is unknown. Characters are gamma-coded
	 *
	 */
	bwt(){}

	/*
	 * Constructor #2
	 *
	 * We know only alphabet size. Each character is assigned log2(sigma) bits.
	 * Characters are assigned codes 0,1,2,... in order of appearance
	 *
	 */
	bwt(uint64_t sigma){

		assert(sigma>0);

		F = rle_string_type(sigma);
		L = dynamic_string_type(sigma);

	}

	/*
	 * Constructor #3
	 *
	 * We know character probabilities. Input: pairs <character, probability>
	 *
	 * Here the alphabet is Huffman encoded.
	 *
	 */
	bwt(vector<pair<char_type,double> >& P){

		for(auto p:P){

			assert(p.first != TERMINATOR);

		}

		F = dynamic_string_type(P);
		L = dynamic_string_type(P);

	}

	/*
	 * i-th BWT character
	 */
	char_type at(ulint i){

		assert(i<bwt_length());

		return 	i < terminator_position ?
				L[i] :
					i == terminator_position ?
					TERMINATOR :
					L[i-1];

	}

	/*
	 * i-th BWT character
	 */
	char_type operator[](ulint i){

		return at(i);

	}

	/*
	 * build BWT(cW) from BWT(W)
	 */
	void extend(char_type c){

		assert(c!=TERMINATOR);

		//position in F where c has to be inserted
		ulint pos_in_F;

		//are we inserting a new character?
		if(alphabet.find(c)==alphabet.end()){

			//iterator to character immediately after c (in lex order)
			//or alphabet.end() if c is bigger than all a in alphabet
			auto upit = alphabet.upper_bound(c);

			pos_in_F = 	upit==alphabet.end() ?
								F.size() :
								F.select(0,*upit);

			alphabet.insert(c);

		}else{

			//number of cs before terminator in L
			ulint c_before = L.rank(terminator_position,c);
			pos_in_F = 	F.select(0,c) + c_before;

		}

		F.insert(pos_in_F,c);

		L.insert(terminator_position,c);

		//add 1 to take into account terminator in F
		terminator_position = pos_in_F+1;

	}

	/*
	 * Input: interval of a string W, and a character c
	 * Output: interval of cW
	 *
	 * Note that intervals are [left,right) : right bound is excluded
	 *
	 */
	pair<ulint, ulint> BS(pair <ulint,ulint> interval, char_type c){

		assert(c!=TERMINATOR);

		assert(interval.first <= bwt_length() and interval.second <= bwt_length());

		/*
		 * if c is not in the alphabet or empty interval, return empty interval
		 */
		if(alphabet.find(c) == alphabet.end() or interval.first >= interval.second)
			return {0,0};

		/*
		 * TERMINATOR is not explicitly stored in L, so
		 * re-map interval coordinates
		 */
		ulint l = 	interval.first <= terminator_position ?
					interval.first :
					interval.first-1;

		ulint r = 	interval.second <= terminator_position ?
					interval.second :
					interval.second-1;

		//position in F of the first c
		//Add 1 because in F.select(0,c) we are
		//not taking into account the terminator (which is in
		//position 0 but not explicitly stored in F)
		ulint F_pos = F.select(0,c) + 1;

		return {	F_pos+L.rank(l,c),
					F_pos+L.rank(r,c)
		};

	}

	/*
	 * LF function
	 */
	ulint LF(ulint i){

		assert(i<bwt_length());

		char_type c = at(i);

		ulint j = i <= terminator_position ? i : i-1;

		return 	c == TERMINATOR ? 0 :
				F.select(0,c) + L.rank(j,c) + 1;

	}

	/*
	 * FL function
	 */
	ulint FL(ulint i){

		assert(i<bwt_length());

		char_type c = i==0 ? TERMINATOR : F[i-1];

		//number of c before position i in F
		ulint j = i==0 ? 0 : F.rank(i-1,c);

		//position on L
		ulint k = 	c == TERMINATOR ? 0 :
					L.select(j,c);

		return 	c == TERMINATOR ? terminator_position :
				k >= terminator_position ? k+1 : k;

	}

	pair<ulint, ulint> get_full_interval(){

		return {0,bwt_length()};

	}

	/*
	 * text length = BWT length-1
	 */
	ulint text_length(){

		return L.size();

	}

	ulint bwt_length(){

		//take into account terminator
		return L.size()+1;

	}

	//alphabet of the text
	ulint text_alphabet_size(){

		return alphabet.size();

	}

	//alphabet of the text + terminator character
	ulint bwt_alphabet_size(){

		return alphabet.size()+1;

	}

	/*
	 * get BWT terminator character
	 */
	char_type get_terminator(){

		return TERMINATOR;

	}

	/*
	 * get position of terminator in the BWT
	 */
	ulint get_terminator_position(){

		return terminator_position;

	}

private:

	/*
	 * First and last BWT matrix columns (L=BWT). Note that these strings
	 * contain all but the terminator characters
	 */
	rle_string_type F;
	dynamic_string_type L;

	//TERMINATOR character: we reserve the integer 2^64-1
	static const char_type TERMINATOR = ~ulint(0);

	//stores the alphabet. Useful to infer lexicographic order
	//of new incoming characters in log sigma time.
	set<char_type> alphabet;

	//terminator is not actually stored in the BWT: we just remember
	//its position
	ulint terminator_position=0;

};

}
#endif /* DYNAMICBWT_H_ */
