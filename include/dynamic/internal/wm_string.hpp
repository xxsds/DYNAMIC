// Copyright (c) 2019, Erik Garrison.  All rights reserved.
// Use of this source code is governed
// by a MIT license that can be found in the LICENSE file.

/*
 * wm_string.hpp
 *
 *  Created on: May 30, 2020
 *      Author: MitI_7
 *      Author: Erik Garrison
 *
 *  Dynamic string supporting rank, select, access, insert.
 *  Backed by a wavelet matrix based on the templated dynamic bitvector.
 *
 */

#ifndef INCLUDE_INTERNAL_WM_STRING_HPP_
#define INCLUDE_INTERNAL_WM_STRING_HPP_

#include "dynamic/internal/includes.hpp"

namespace dyn{

template<class dynamic_bitvector_t>

class wm_string{

public:
    std::vector<dynamic_bitvector_t> bit_arrays;
    std::vector<ulint> begin_one;               // 各bitの1の開始位置

    ulint n = 0;                                    // 与えられた配列のサイズ
    ulint sigma = 0;                      // 最大の文字
    ulint bit_width = 0;                             // 文字を表すのに必要なbit数

public:
    wm_string (void) { } // default constructor, for loading from file

    // max_element: 入ってくる中で一番大きい数値
    wm_string (ulint sigma) : n(0), sigma(sigma + 1) {
        this->bit_width = this->get_num_of_bit(sigma);
        if (bit_width == 0) {
            bit_width = 1;
        }
        this->begin_one.resize(bit_width);

        for (ulint i = 0; i < bit_width; ++i) {
            dynamic_bitvector_t sv;
            bit_arrays.push_back(sv);
        }
    }

    // hack to accept an alphabet with frequencies, to match wt_string interface for testing
    // doesn't work
    /*
    wm_string(vector<pair<ulint, double>>& P) {
        n = 0;
        sigma = 0;
        for (auto& e : P) {
            if (e.first > sigma) {
                sigma = e.first;
            }
        }
        ++sigma;
        this->bit_width = this->get_num_of_bit(sigma-1);
        if (bit_width == 0) {
            bit_width = 1;
        }
        this->begin_one.resize(bit_width);

        for (ulint i = 0; i < bit_width; ++i) {
            dynamic_bitvector_t sv;
            bit_arrays.push_back(sv);
        }
    }
    */

    wm_string (ulint num_of_alphabet, const std::vector<ulint> &array) : n(0), sigma(num_of_alphabet + 1) {
        this->bit_width = this->get_num_of_bit(num_of_alphabet);
        if (bit_width == 0) {
            bit_width = 1;
        }
        this->begin_one.resize(bit_width);

        if (array.empty()) {
            for (ulint i = 0; i < bit_width; ++i) {
                dynamic_bitvector_t sv;
                bit_arrays.push_back(sv);
            }
            return;
        }

        n = array.size();

        std::vector<ulint> v(array), b(array.size(), 0);

        for (ulint i = 0; i < bit_width; ++i) {

            std::vector<ulint> temp;
            // 0をtempにいれてく
            for (ulint j = 0; j < v.size(); ++j) {
                ulint c = v.at(j);
                ulint bit = (c >> (bit_width - i - 1)) & 1;  //　上からi番目のbit
                if (bit == 0) {
                    temp.push_back(c);
                    b[j] = 0;
                }
            }

            this->begin_one.at(i) = temp.size();

            // 1をtempにいれてく
            for (ulint j = 0; j < v.size(); ++j) {
                ulint c = v.at(j);
                ulint bit = (c >> (bit_width - i - 1)) & 1;  //　上からi番目のbit
                if (bit == 1) {
                    temp.push_back(c);
                    b[j] = 1;
                }
            }

            dynamic_bitvector_t dbv;
            for (auto& i : b) { dbv.push_back(i); }
            bit_arrays.emplace_back(dbv);
            v = temp;
        }
    }

    ulint serialize(ostream& out) const {
        ulint w_bytes = 0;
        out.write((char*)&n, sizeof(n));
        w_bytes += sizeof(n);
        out.write((char*)&sigma, sizeof(sigma));
        w_bytes += sizeof(sigma);
        out.write((char*)&bit_width, sizeof(bit_width));
        w_bytes += sizeof(bit_width);
        out.write((char*)begin_one.data(), sizeof(ulint) * bit_width);
        w_bytes += sizeof(ulint) * bit_width;
        for (auto& bv : bit_arrays) {
            w_bytes += bv.serialize(out);
        }
        return w_bytes;
    }

    void load(istream& in) {
        in.read((char*)&n, sizeof(n));
        in.read((char*)&sigma, sizeof(sigma));
        in.read((char*)&bit_width, sizeof(bit_width));
        begin_one.resize(bit_width);
        in.read((char*)begin_one.data(), sizeof(ulint) * bit_width);
        bit_arrays.resize(bit_width);
        for (ulint i = 0; i < bit_width; ++i) {
            bit_arrays[i].load(in);
        }
    }

    // v[pos]
    ulint at(ulint pos) const {
        assert(pos < this->n);

        ulint c = 0;
        for (ulint i = 0; i < bit_arrays.size(); ++i) {
            ulint bit = bit_arrays.at(i).at(pos);   // もとの数値がのi番目のbit
            c = (c <<= 1) | bit;
            pos = bit_arrays.at(i).rank(pos, bit);
            if (bit) {
                pos += this->begin_one.at(i);
            }
        }
        return c;
    }

    // high-level access
    ulint operator[](ulint i) const { return this->at(i); }

    // v[0, pos)のcの数
    ulint rank(ulint pos, ulint c) const {
        assert(pos <= n);
        if (c >= sigma) {
            return 0;
        }

        ulint left = 0, right = pos;
        for (ulint i = 0; i < bit_width; ++i) {
            const ulint bit = (c >> (bit_width - i - 1)) & 1;  // 上からi番目のbit
            left = bit_arrays.at(i).rank(left, bit);             // cのi番目のbitと同じ数値の数
            right = bit_arrays.at(i).rank(right, bit);           // cのi番目のbitと同じ数値の数
            if (bit) {
                left += this->begin_one.at(i);
                right += this->begin_one.at(i);
            }
        }

        return right - left;
    }

    // i番目のcの位置 + 1を返す。rankは1-origin
    ulint select(ulint rank, ulint c) const {
        assert(rank > 0);
        assert(c < sigma);
        --rank;

        ulint left = 0;
        for (ulint i = 0; i < bit_width; ++i) {
            const ulint bit = (c >> (bit_width - i - 1)) & 1;  // 上からi番目のbit
            left = bit_arrays.at(i).rank(left, bit);               // cのi番目のbitと同じ数値の数
            if (bit) {
                left += this->begin_one.at(i);
            }
        }

        ulint index = left + rank;
        for (ulint i = 0; i < bit_arrays.size(); ++i){
            ulint bit = ((c >> i) & 1);      // 下からi番目のbit
            if (bit == 1) {
                index -= this->begin_one.at(bit_width - i - 1);
            }
            //std::cerr << "selecting index " << index << " bit " << bit << std::endl;
            index = this->bit_arrays.at(bit_width - i - 1).select(index, bit);
        }
        return index+1;
    }

    // posにcを挿入する
    void insert(ulint pos, ulint c) {
        assert(pos <= this->n);

        for (ulint i = 0; i < bit_arrays.size(); ++i) {
            const ulint bit = (c >> (bit_width - i - 1)) & 1;  //　上からi番目のbit
            bit_arrays.at(i).insert(pos, bit);
            pos = bit_arrays.at(i).rank(pos, bit);
            if (bit) {
                pos += this->begin_one.at(i);
            }
            else {
                this->begin_one.at(i)++;
            }
        }

        this->n++;
    }

    void push_front(ulint c) {
        this->insert(0, c);
    }

    // 末尾にcを追加する
    void push_back(ulint c) {
        this->insert(this->n, c);
    }

    // posを削除する
    void remove(ulint pos) {
        assert(pos < this->n);
        if (pos >= this->n) {
            throw "Segmentation fault";
        }

        for (ulint i = 0; i < bit_arrays.size(); ++i) {
            ulint bit = bit_arrays.at(i).at(pos);   // もとの数値のi番目のbit

            auto next_pos = bit_arrays.at(i).rank(pos, bit);
            bit_arrays.at(i).remove(pos);

            if (bit) {
                next_pos += this->begin_one.at(i);
            }
            else {
                this->begin_one.at(i)--;
            }
            pos = next_pos;
        }
        this->n--;
    }

    // posにcをセットする
    void update(ulint pos, ulint c) {
        assert(pos < this->n);
        this->remove(pos);
        this->insert(pos, c);
    }

    ulint size(void) const {
        return this->n;
    }

    ulint bit_size(void) const {
        ulint n_bits = 0;
        for (auto& ba : bit_arrays) {
            n_bits += ba.bit_size();
        }
        n_bits += sizeof(ulint) * begin_one.size();
        return n_bits;
    }

    // 他の操作は通常のWavelet Matrixと同じ

private:
    ulint get_num_of_bit(ulint x) {
        if (x == 0) return 0;
        x--;
        ulint bit_num = 0;
        while (x >> bit_num) {
            ++bit_num;
        }
        return bit_num;
    }
};

}


#endif /* INCLUDE_INTERNAL_WM_STRING_HPP_ */
