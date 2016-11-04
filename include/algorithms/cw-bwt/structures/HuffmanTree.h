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
 * HuffmanTree.h
 *
 *  Created on: Jun 15, 2014
 *      Author: nicola
 */

#ifndef HUFFMANTREE_H_
#define HUFFMANTREE_H_

#include "includes.hpp"

namespace bwtil {

template<typename T = symbol>
class HuffmanTree {
public:

	//a node of the tree
	class Node{

	public:

		Node(T label, ulint freq){
			this->freq=freq;
			this->label=label;
			leaf=true;
		}

		Node(T nr, Node l, Node r){

			node_number=nr;
			freq = l.freq+r.freq;
			leaf=false;
			leaf_right = r.leaf;
			leaf_left = l.leaf;

			if(leaf_left)
				left = l.label;
			else
				left = l.node_number;

			if(leaf_right)
				right = r.label;
			else
				right = r.node_number;

		}

		bool operator<(const Node & n) const { return freq<n.freq; };

		T node_number;
		ulint freq;
		T label;//label if leaf
		bool leaf = 0;//this node is leaf
		bool leaf_left = 0;
		bool leaf_right = 0;

		T left;//pointers to children
		T right;

	};

	HuffmanTree(){};

	//freq = array containing absolute number of occurrencies of each T {0,...,freq.size()-1}
	HuffmanTree(vector<ulint> freq){

		sigma_0 = freq.size();

		//sigma = 0;//number of Ts with frequency > 0

		frequencies = freq;//copy freq

		ulint tot=0;

		for(uint i=0;i<sigma_0;i++){

			frequencies.at(i) = freq.at(i);

			tot+=frequencies.at(i);

			//if(freq.at(i)>0)
				//sigma++;

		}

		if(tot==0){
			cout << "Error (HuffmanTree constructor) : Empty Huffman tree.\n";
			exit(0);
		}

		codes = vector<vector<bool> >(sigma_0);//code associated to each T (empty vector if T has freq=0)

		//build Huffman tree using objects, then copy it in a more compact format inside the above vectors

		multiset<Node> nodes;
		vector<Node> nodes_vec;//here nodes (only internal nodes) are stored but not deleted.

		//create leafs
		for(uint i=0;i<sigma_0;i++)
			if(frequencies.at(i)>0)
				nodes.insert(Node(i,frequencies.at(i)));

		//Huffman's algorithm

		while(nodes.size()>1){//repeat until all trees are merged

			Node min1 = *nodes.begin();//extract the 2 smallest nodes
			nodes.erase(nodes.begin());//erase the 2 smallest nodes

			Node min2 = *nodes.begin();
			nodes.erase(nodes.begin());

			nodes.insert(Node(nodes_vec.size(),min1,min2));
			nodes_vec.push_back(Node(nodes_vec.size(),min1,min2));

		}

		//save the tree in arrays
		storeTree(*nodes.begin(),nodes_vec);

		//free memory
		nodes.clear();
		nodes_vec.clear();

	}

	double entropy(){//get entropy

		double entropy = 0;
		double tot=0;

		for(uint i=0;i<sigma_0;i++)
			tot+=frequencies.at(i);

		if(tot==0){
			cout << "Error (HuffmanTree entropy): Empty Huffman tree.\n";
			exit(0);
		}

		for(uint i=0;i<sigma_0;i++){

			double l = codes[i].size();
			double f = (double)frequencies.at(i)/tot;

			entropy += l*f;

		}

		return entropy;

	}

	ulint numberOfOccurrencies(T s){return frequencies.at(s);};//number of occurrencies of the T s

	vector<vector<bool> > getCodes(){return codes;}

	vector<bool> code(T s){return codes[s];};//from T -> to its binary Huffman code (compression)

private:

	void storeTree(Node n,vector<Node> nodes_vec){

			//root_node = n.node_number;
			storeTree(vector<bool>(),n,nodes_vec);

		}

	void storeTree(vector<bool> code, Node n,vector<Node> nodes_vec){

		vector<bool> codel = code;
		vector<bool> coder = code;

		codel.push_back(0);
		coder.push_back(1);

		//T node_nr = n.node_number;

		if(n.leaf_left){//leaf

			codes[n.left] = codel;
			//left_leafs.at(node_nr) = true;
			//left[node_nr] = n.left;

		}else{

			//left_leafs.at(node_nr) = false;
			//left[node_nr] = n.left;

			storeTree(codel,nodes_vec.at(n.left), nodes_vec);

		}

		if(n.leaf_right){//leaf

			codes[n.right] = coder;
			//right_leafs.at(node_nr) = true;
			//right[node_nr] = n.right;

		}else{

			//right_leafs.at(node_nr) = false;
			//right[node_nr] = n.right;

			storeTree(coder,nodes_vec.at(n.right), nodes_vec);

		}

	}

	//T decode(vector<bool> c, uint pos, T node);

	T sigma_0;//nr of Ts
	//T sigma;//nr of Ts with frequency > 0

	//the Huffman tree:
	//T root_node;
	//T * left;//sigma-1 left pointers
	//T * right;//sigma-1 right pointers
	//vector<bool> left_leafs;//for each internal node i, memorizes if the T in left is pointer to internal nodes (0) or leaf label (1)
	//vector<bool> right_leafs;//for each internal node i, memorizes if the T in left is pointer to internal nodes (0) or leaf label (1)

	vector<ulint> frequencies;//table of sigma_0 entries. Symbol -> number of occurrencies.

	vector<vector<bool> > codes;

};

} /* namespace compressed_bwt_construction */
#endif /* HUFFMANTREE_H_ */
