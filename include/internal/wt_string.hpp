// Copyright (c) 2017, Nicola Prezza.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

/*
 * wt_string.hpp
 *
 *  Created on: Nov 30, 2015
 *      Author: nico
 *
 *  Dynamic string supporting rank, select, access, insert.
 *
 *  The string uses a wavelet tree. The encoding depends on the constructor:
 *
 *  - dynamic_string() : gamma coding. maximum code bit-length will be 2log
 * sigma. New characters can always be inserted.
 *  - dynamic_string(uint64_t sigma) : fixed-length. Max sigma characters are
 * allowed
 *  - dynamic_string(vector<pair<char_type,double> > P) : Huffman-encoding. The
 * characters set is fixed at construction time.
 *
 */

#ifndef INCLUDE_INTERNAL_WT_STRING_HPP_
#define INCLUDE_INTERNAL_WT_STRING_HPP_

#include "alphabet_encoder.hpp"
#include "includes.hpp"

namespace dyn {

template <class dynamic_bitvector_t>
class wt_string {
 public:
  // we allow any alphabet
  typedef uint64_t char_type;
  typedef char_type value_type;

  /*
   * Constructor #1
   *
   * Alphabet is unknown. Characters are gamma-coded
   *
   */
  wt_string() {}

  /*
   * Constructor #2
   *
   * We know only alphabet size. Each character is assigned log2(sigma) bits.
   * Characters are assigned codes 0,1,2,... in order of appearance
   *
   */
  explicit wt_string(uint64_t sigma) {
    assert(sigma > 0);
    ae = alphabet_encoder(sigma);
  }

  /*
   * Constructor #3
   *
   * We know character probabilities. Input: pairs <character, probability>
   *
   * Here the alphabet is Huffman encoded.
   *
   */
  explicit wt_string(vector<pair<char_type, double>>& P) { ae = alphabet_encoder(P); }

  template <typename t_str>
  wt_string(uint64_t sigma, const t_str& str) : wt_string(sigma) { push_many(str); }

  /*
   * number of bits in the bitvector
   */
  uint64_t size() const { return n; }

  /*
   * high-level access to the string.
   */
  char_type operator[](uint64_t i) const { return at(i); }

  /*
   * access
   */
  char_type at(uint64_t i) const {
    assert(i < size());
    return root.at(i);
  }

  /*
   * position of i-th character equal to c. 0 =< i < rank(size(),c)
   */
  uint64_t select(uint64_t i, char_type c) const {
    assert(ae.char_exists(c));
    assert(i < rank(size(), c));

    const auto& code = ae.encode_existing(c);

    // if this fails, it means that c is not present
    // in the string or that it is not present
    // in the initial dictionary (if any)
    assert(code.size() > 0);

    // this code must have been inserted in the tree already
    assert(root.exists(code));

    return root.select(i, code);
  }

  /*
   * number of chars equal to c before position i EXCLUDED
   */
  uint64_t rank(uint64_t i, char_type c) const {
    assert(i <= size());

    if (not ae.char_exists(c)) return 0;

    const auto& code = ae.encode_existing(c);

    // if this fails, it means that c is not present
    // in the string or that it is not present
    // in the initial dictionary (if any)
    assert(code.size() > 0);

    /*
     * if condition is false, it means that the code is in the
     * alphabet encoder, but it has not yet been inserted
     * in the tree
     */
    return root.exists(code) ? root.rank(i, code) : 0;
  }

  bool char_exists(char_type c) const { return ae.char_exists(c); }

  void push_back(char_type c) { insert(size(), c); }

  // insert values from range [0,...,sigma)
  template <class Vector>
  void push_many(uint64_t sigma, const Vector& values) {
    map<char_type, vector<bool>> path_to_leaf;
    for (ulint c = 0; c < sigma; ++c) {
      path_to_leaf[c] = ae.encode(c);
    }

    assert(std::all_of(values.begin(), values.end(),
                       [&](typename Vector::value_type c) { return ae.char_exists(c); }));

    #pragma omp parallel
    #pragma omp master
    root.push_many(path_to_leaf, values);
    n += values.size();
  }

  template <class Vector>
  void push_many(const Vector& values) {
    for (ulint i = 0; i < values.size(); ++i) {
      auto c = values[i];
      if (!ae.char_exists(c)) ae.encode(c);
    }

    map<char_type, vector<bool>> path_to_leaf;
    for (char_type c : ae.keys()) {
      path_to_leaf[c] = ae.encode(c);
    }

    #pragma omp parallel
    #pragma omp master
    root.push_many(path_to_leaf, values);
    n += values.size();
  }

  void push_front(char_type c) { insert(0, c); }

  /*
   * insert a character at position i
   */
  void insert(uint64_t i, char_type c) {
    // get code of c
    // if code does not yet exist, create it
    const auto& code = ae.encode(c);

    root.insert(i, code, c);

    ++n;
  }

  /*
   * remove character at position i
   */
  void remove(uint64_t i) {
    char_type c = this->at(i);
    // get code of c
    const auto& code = ae.encode(c);

    root.remove(i, code, c);
    --n;
  }

  uint64_t bit_size() const {
    uint64_t size = 0;
    size += sizeof(wt_string<dynamic_bitvector_t>) * 8;
    size += ae.bit_size();
    size += root.bit_size();
    return size;
  }

  ulint alphabet_size() const { return ae.size(); }

  ulint serialize(ostream& out) const {
    ulint w_bytes = 0;

    out.write((char*)&n, sizeof(n));
    w_bytes += sizeof(n);

    w_bytes += root.serialize(out);

    w_bytes += ae.serialize(out);

    return w_bytes;
  }

  void load(istream& in) {
    in.read((char*)&n, sizeof(n));
    root.load(in);
    ae.load(in);
  }

 private:
  class node;

  // current length
  ulint n = 0;

  node root;

  alphabet_encoder ae;
};

template <class dynamic_bitvector_t>
class wt_string<dynamic_bitvector_t>::node {
 public:
  // root constructor
  node() {}

  // parent constructor
  explicit node(node* parent_) { this->parent_ = parent_; }

  /*
   * copy constructor
   */
  explicit node(const node& other)
      : child0_(nullptr),
        child1_(nullptr),
        parent_(nullptr),
        l_(0),
        is_leaf_(false) {
    *this = other;
  }

  ~node() {
    if (has_child0()) {
      delete child0_;
      child0_ = NULL;
    }

    if (has_child1()) {
      delete child1_;
      child1_ = NULL;
    }
  }

  node& operator=(const node& other) {
    if (!child0_) child0_ = new node();
    if (other.child0_) {
      *child0_ = *other.child0_;
      child0_->parent_ = this;
    } else {
      delete child0_;
      child0_ = nullptr;
    }
    if (!child1_) child1_ = new node();
    if (other.child1_) {
      *child1_ = *other.child1_;
      child1_->parent_ = this;
    } else {
      delete child1_;
      child1_ = nullptr;
    }
    parent_ = other.parent_;
    bv = other.bv;
    l_ = other.l_;
    is_leaf_ = other.is_leaf_;
    return *this;
  }

  node(node&& other)
      : child0_(nullptr),
        child1_(nullptr),
        parent_(nullptr),
        l_(0),
        is_leaf_(false) {
    *this = std::move(other);
  }

  node& operator=(node&& other) {
    if (this != &other) {
      if (has_child0()) delete child0_;
      if (has_child1()) delete child1_;
      child0_ = other.child0_;
      if (has_child0()) child0_->parent_ = this;
      child1_ = other.child1_;
      if (has_child1()) child1_->parent_ = this;
      parent_ = other.parent_;
      bv = std::move(other.bv);
      l_ = other.l_;
      is_leaf_ = other.is_leaf_;
      other.child0_ = nullptr;
      other.child1_ = nullptr;
      other.parent_ = nullptr;
    }
    return *this;
  }

  // turn this node into a leaf
  void make_leaf(char_type c) {
    assert(not has_child0());  // musttnot have children
    assert(not has_child1());
    assert(bv.size() == 0);

    assert(not is_root());  // cannot make the root leaf

    this->is_leaf_ = true;
    this->l_ = c;
  }

  // descend tree and return character at this position
  char_type at(ulint i) const {
    if (is_leaf()) return label();

    assert(i < bv.size());

    bool b = bv.at(i);

    assert((b or has_child0()) and (not b or has_child1()));

    if (b) {
      return child1_->at(bv.rank1(i));
    }

    return child0_->at(bv.rank0(i));
  }

  /*
   * true iif code B has already been inserted
   */
  bool exists(const vector<bool>& B, ulint j = 0) const {
    assert(j <= B.size());

    // not at the end of B -> (B[j] -> must have child 1)
    bool c1 = j == B.size() || (not B[j] || has_child1());

    // not at the end of B -> (notB[j] -> must have child 0)
    bool c0 = j == B.size() || (B[j] || has_child0());

    return (c1 and c0) &&
           (j == B.size() ? true
                          : (B[j] ? child1_->exists(B, j + 1)
                                  : child0_->exists(B, j + 1)));
  }

  /*
   * insert code B[j,...,B.size()-1] at position i. This code is associated
   * with character c
   */
  void insert(ulint i, const vector<bool>& B, char_type c, ulint j = 0) {
    if (j == B.size()) {
      // this node must be a leaf
      assert(bv.size() == 0);

      if (is_leaf()) {
        // if it's already marked as leaf, check
        // that the label is correct
        assert(c == label());

      } else {
        // else, mark node as leaf
        make_leaf(c);
      }

      return;
    }

    assert(i <= bv.size());
    assert(not is_leaf());

    bool b = B[j];
    bv.insert(i, b);

    if (b) {
      // if child does not exist, create it.
      if (not has_child1()) {
        child1_ = new node(this);
      }

      assert(j == B.size() - 1 or bv.rank1(i) <= child1_->bv.size());

      child1_->insert(bv.rank1(i), B, c, j + 1);

    } else {
      if (not has_child0()) {
        child0_ = new node(this);
      }

      assert(j == B.size() - 1 or bv.rank0(i) <= child0_->bv.size());

      child0_->insert(bv.rank0(i), B, c, j + 1);
    }
  }

  template <class Vector>
  void push_many(const map<char_type, vector<bool>>& Bs, const Vector& values,
                 ulint j = 0, ulint offset = 0) {
    if (Bs.size() == 1) {
      // this node must be a leaf
      assert(bv.size() == 0);

      auto c = Bs.begin()->first;
      assert(j == Bs.at(c).size());

      if (is_leaf()) {
        // if it's already marked as leaf, check
        // that the label is correct
        assert(c == label());
      } else {
        // else, mark node as leaf
        make_leaf(c);
      }
      return;
    }

    assert(not is_leaf());

    bool task_started_0 = false;
    bool task_started_1 = false;

    tsl::hopscotch_map<char_type, bool> assignment;
    map<char_type, vector<bool>> Bs_left, Bs_right;
    for_each(Bs.begin(), Bs.end(),
             [&](const pair<char_type, vector<bool>>& pair) {
               if (pair.second[j]) {
                 Bs_right.insert(pair);
               } else {
                 Bs_left.insert(pair);
               }
               assignment[pair.first] = pair.second[j];
             });

    uint64_t word = 0;
    uint8_t num_bits = 0;

    for (ulint idx = offset; idx < values.size(); ++idx) {
      char_type c = values[idx];
      auto it = assignment.find(c);
      if (it == assignment.end()) continue;

      uint64_t b = it->second;

      word |= b << num_bits;

      if (++num_bits == 64) {
        bv.push_word(word, 64);
        word = 0;
        num_bits = 0;
      }

      if (b && !task_started_1) {
        task_started_1 = true;

        if (not has_child1()) child1_ = new node(this);

        #pragma omp task shared(values, Bs_right)
        child1_->push_many(Bs_right, values, j + 1, idx);
      }

      if (!b && !task_started_0) {
        task_started_0 = true;

        if (not has_child0()) child0_ = new node(this);

        #pragma omp task shared(values, Bs_left)
        child0_->push_many(Bs_left, values, j + 1, idx);
      }
    }

    if (num_bits) bv.push_word(word, num_bits);

    #pragma omp taskwait
  }

  /*
   * remove code B[j,...,B.size()-1] from position i. This code is associated
   * with character c
   */
  void remove(ulint i, const vector<bool>& B, char_type c, ulint j = 0) {
    if (j == B.size()) {
      // TODO: Check if leaf should be deleted?

      // this node must be a leaf
      assert(bv.size() == 0);
      assert(c == label());
      // if(is_leaf()){

      //    //if it's already marked as leaf, check
      //    //that the label is correct
      //    assert(c==label());

      // }else{

      //    //else, mark node as leaf
      //    make_leaf(c);

      // }

      return;
    }

    assert(i <= (bv.size() - 1));
    assert(not is_leaf());

    bool b = B[j];

    assert(b == bv.at(i));

    if (b) {
      // TODO: delete empty nodes?

      assert(j == B.size() - 1 or bv.rank1(i) <= child1_->bv.size());

      child1_->remove(bv.rank1(i), B, c, j + 1);

    } else {
      assert(j == B.size() - 1 or bv.rank0(i) <= child0_->bv.size());

      child0_->remove(bv.rank0(i), B, c, j + 1);
    }

    bv.remove(i);
  }

  ulint rank(ulint i, const vector<bool>& B, ulint j = 0) const {
    assert(j <= B.size());

    // not at the end of B -> (B[j] -> must have child 1)
    assert(j == B.size() || (not B[j] || has_child1()));

    // not at the end of B -> (notB[j] -> must have child 0)
    assert(j == B.size() || (B[j] || has_child0()));

    // not at the end of B -> i must be smaller than bv.size()
    assert(j == B.size() || i <= bv.size());

    return j == B.size() ? i
                         : (B[j] ? child1_->rank(bv.rank1(i), B, j + 1)
                                 : child0_->rank(bv.rank0(i), B, j + 1));
  }

  ulint select(ulint i, const vector<bool>& B) const {
    // top-down: find leaf associated with B
    const node* L = get_leaf(B);

    auto p = L->parent_;

    // bottom-up: from leaf to root
    return p->select(i, B, B.size() - 1);
  }

  ulint select(ulint i, const vector<bool>& B, ulint j) {
    auto s = bv.select(i, B[j]);

    return j == 0 ? s : parent_->select(s, B, j - 1);
  }

  // get leaf associated to code B
  const node* get_leaf(const vector<bool>& B, ulint j = 0) const {
    assert(j <= B.size());

    return j == B.size() ? this
                         : (B[j] ? child1_->get_leaf(B, j + 1)
                                 : child0_->get_leaf(B, j + 1)

                           );
  }

  bool is_root() const { return not parent_; }
  bool is_leaf() const { return is_leaf_; }
  bool has_child0() const { return child0_; }
  bool has_child1() const { return child1_; }

  /*
   * to have left label, this node must not have a left (0)
   * child AND must have at least one bit equal to 0 in
   * its bitvector
   */
  char_type label() const {
    assert(is_leaf());
    return l_;
  }

  /*
   * Total number of bits allocated in RAM for this structure
   *
   * WARNING: this measure is good only for relatively small alphabets (e.g.
   * ASCII) as we use STL containers such as set and map which do not give
   * direct info on the total memory allocated. The sizes of these containers
   * are proportional to the alphabet size (but the constants involved are
   * high since internally they can use heavy structures as RBT)
   */
  ulint bit_size() const {
    ulint size = sizeof(node) * 8;

    size += bv.bit_size();
    if (child0_ != NULL) size += child0_->bit_size();
    if (child1_ != NULL) size += child1_->bit_size();

    return size;
  }

  ulint serialize(ostream& out) const {
    ulint w_bytes = 0;

    out.write((char*)&l_, sizeof(l_));
    w_bytes += sizeof(l_);

    out.write((char*)&is_leaf_, sizeof(is_leaf_));
    w_bytes += sizeof(is_leaf_);

    w_bytes += bv.serialize(out);

    bool has_child0 = child0_ != NULL;
    bool has_child1 = child1_ != NULL;

    out.write((char*)&has_child0, sizeof(has_child0));
    w_bytes += sizeof(has_child0);

    out.write((char*)&has_child1, sizeof(has_child1));
    w_bytes += sizeof(has_child1);

    if (child0_) w_bytes += child0_->serialize(out);
    if (child1_) w_bytes += child1_->serialize(out);

    return w_bytes;
  }

  void load(istream& in) {
    in.read((char*)&l_, sizeof(l_));

    in.read((char*)&is_leaf_, sizeof(is_leaf_));

    bv.load(in);

    bool has_child0;
    bool has_child1;

    in.read((char*)&has_child0, sizeof(has_child0));
    in.read((char*)&has_child1, sizeof(has_child1));

    child0_ = NULL;
    child1_ = NULL;

    if (has_child0) {
      child0_ = new node();
      child0_->load(in);
      child0_->parent_ = this;
    }

    if (has_child1) {
      child1_ = new node();
      child1_->load(in);
      child1_->parent_ = this;
    }
  }

 private:
  node* child0_ = NULL;
  node* child1_ = NULL;
  node* parent_ = NULL;  // parent (NULL if root)

  dynamic_bitvector_t bv;

  // if is_leaf_, then node is labeled
  char_type l_ = 0;
  bool is_leaf_ = false;
};

}  // namespace dyn

#endif /* INCLUDE_INTERNAL_WT_STRING_HPP_ */
