#include <bits/stdc++.h>
#include <random>
#include "dynamic/dynamic.hpp"

using namespace std;

double speed_access(uint64_t num, uint64_t num_of_alphabet) {
    vector<uint64_t> data(num);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(0, num_of_alphabet-1);
    for (int i = 0; i < num; ++i) {
        data[i] = alpha_distrib(gen);
    }

    dyn::wm_str wm(num_of_alphabet, data);

    int dummy = 0;
    auto start = std::chrono::system_clock::now();
    for (int i = 0; i < data.size(); ++i) {
        if (wm.at(i) == -1) {
            dummy++;
        }
    }
    auto end = std::chrono::system_clock::now();
    if (dummy) {
        cout << "dummy" << endl;
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

double speed_rank(uint64_t num, uint64_t num_of_alphabet) {
    vector<uint64_t> data(num);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(0, num_of_alphabet-1);
    for (int i = 0; i < num; ++i) {
        data[i] = alpha_distrib(gen);
    }

    dyn::wm_str wm(num_of_alphabet, data);

    std::uniform_int_distribution<uint64_t> data_distrib(0, num-1);
    int dummy = 0;
    auto start = std::chrono::system_clock::now();
    for (uint64_t rank = 1; rank < data.size(); ++rank) {
        uint64_t val = data[data_distrib(gen)];
        if (wm.rank(rank, val) == -1) {
            dummy++;
        }
    }
    auto end = std::chrono::system_clock::now();
    if (dummy) {
        cout << "dummy" << endl;
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

double speed_select(uint64_t num, uint64_t num_of_alphabet) {
    vector<uint64_t> data(num), count(num_of_alphabet, 0);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(0, num_of_alphabet-1);
    for (int i = 0; i < num; ++i) {
        data[i] = alpha_distrib(gen);
        count[data[i]]++;
    }

    dyn::wm_str wm(num_of_alphabet, data);

    std::uniform_int_distribution<uint64_t> data_distrib(0, num-1);
    int dummy = 0;
    auto start = std::chrono::system_clock::now();
    for (int i = 0; i < num; ++i) {
        uint64_t val = data[data_distrib(gen)];
        std::uniform_int_distribution<uint64_t> rank_distrib(1, count.at(val));
        uint64_t rank  = rank_distrib(gen);
        //std::cerr << "select(" << rank << ", " << val << ")" << std::endl;
        if (wm.select(rank, val) == -1) {
            dummy++;
        }
    }
    auto end = std::chrono::system_clock::now();
    if (dummy) {
        cout << "dummy" << endl;
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

double speed_remove(uint64_t num, uint64_t num_of_alphabet) {
    vector<uint64_t> data;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(0, num_of_alphabet-1);
    for (int i = 0; i < num; ++i) {
        data.emplace_back(alpha_distrib(gen));
    }

    dyn::wm_str dwm(num_of_alphabet, data);

    auto start = std::chrono::system_clock::now();
    for (int i = 0; i < num; ++i) {
        std::uniform_int_distribution<uint64_t> pos_distrib(0, dwm.size()-1);
        uint64_t pos = pos_distrib(gen);
        dwm.remove(pos);
    }
    auto end = std::chrono::system_clock::now();
    assert(dwm.size() == 0);

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

double speed_insert(uint64_t num, uint64_t num_of_alphabet) {

    dyn::wm_str dwm(num_of_alphabet);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(0, num_of_alphabet-1);

    auto start = std::chrono::system_clock::now();
    for (int i = 0; i < num; ++i) {
        std::uniform_int_distribution<uint64_t> pos_distrib(0, dwm.size());
        uint64_t pos = pos_distrib(gen);
        uint64_t c = alpha_distrib(gen);
        dwm.insert(pos, c);
    }
    auto end = std::chrono::system_clock::now();
    if (dwm.at(0) == -1) {
        cout << "dummy" << endl;
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

double speed_update(uint64_t num, uint64_t num_of_alphabet) {

    vector<uint64_t> data(num);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(0, num_of_alphabet-1);
    for (int i = 0; i < num; ++i) {
        data[i] = alpha_distrib(gen);
    }

    dyn::wm_str dwm(num_of_alphabet, data);
    std::uniform_int_distribution<uint64_t> pos_distrib(0, dwm.size()-1);

    auto start = std::chrono::system_clock::now();
    for (int i = 0; i < num - 1; ++i) {
        uint64_t pos = pos_distrib(gen);
        uint64_t c = alpha_distrib(gen);
        dwm.update(pos, c);
    }
    auto end = std::chrono::system_clock::now();
    if (dwm.at(0) == -1) {
        cout << "dummy" << endl;
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

bool test_access(uint64_t num, uint64_t num_of_alphabet) {
    vector<uint64_t> data(num);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(0, num_of_alphabet-1);
    for (int i = 0; i < num; ++i) {
        data[i] = alpha_distrib(gen);
    }

    dyn::wm_str wm(num_of_alphabet, data);
    for (int i = 0; i < data.size(); ++i) {
        if (data[i] != wm.at(i)) {
            return false;
        }
    }
    return true;
}

bool test_rank(uint64_t num, uint64_t num_of_alphabet) {
    vector<uint64_t> data(num);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(0, num_of_alphabet-1);
    for (int i = 0; i < num; ++i) {
        data[i] = alpha_distrib(gen);
    }

    dyn::wm_str wm(num_of_alphabet, data);

    set<uint64_t> s(data.begin(), data.end());
    for (auto val : s) {
        for (uint64_t i = 0; i < data.size(); ++i) {

            int expected = 0;
            for (int j = 0; j < i; ++j) {
                expected += (data[j] == val);
            }

            if (wm.rank(i, val) != expected) {
                return false;
            }
        }
    }

    return true;
}


bool test_select(uint64_t num, uint64_t num_of_alphabet) {
    vector<uint64_t> data(num), count(num_of_alphabet, 0);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(0, num_of_alphabet-1);
    for (int i = 0; i < num; ++i) {
        data[i] = alpha_distrib(gen);
        count[data[i]]++;
    }

    dyn::wm_str wm(num_of_alphabet, data);

    set<uint64_t> s(data.begin(), data.end());
    for (auto val : s) {
        //cerr << "selecting " << val << endl;
        for (uint64_t rank = 1; rank < count.at(val); ++rank) {

            uint64_t expected = 0;
            int n = 0;
            for (uint64_t i = 0; i < data.size(); ++i) {
                n += (data.at(i) == val);
                if (n == rank) {
                    expected = i + 1;   // selectは見つけたindex+1
                    break;
                }
            }

            //cerr << "val/rank " << val << " " << rank << endl;
            if (wm.select(rank, val) != expected) {
                //cerr << "got " << wm.select(val, rank) << " but expected " << expected << endl;
                return false;
            } else {
                //cerr << "got " << wm.select(val, rank) << endl;
            }
        }
    }

    return true;
}

// 2つの動的ビットベクトルが同じかチェック
bool same(dyn::wm_str expected, dyn::wm_str actual) {

    if (expected.size() != actual.size()) {
        cout << "Error at n:" << " expected:" << expected.size() << " actual: " << actual.size() << endl;
        return false;
    }
    if (expected.sigma != actual.sigma) {
        cout << "Error at sigma:" << " expected:" << expected.sigma << " actual: " << actual.sigma << endl;
        return false;
    }
    if (expected.bit_width != actual.bit_width) {
        cout << "Error at num_of_bit:" << " expected:" << expected.bit_width << " actual: " << actual.bit_width << endl;
        return false;
    }
    if (expected.begin_one != actual.begin_one) {
        cout << "Error at begin_one" << endl;
        assert(expected.begin_one.size() == actual.begin_one.size());
        for (int i = 0; i < expected.begin_one.size(); ++i) {
            if (expected.begin_one.at(i) != actual.begin_one.at(i)) {
                cout << "begin_one[" << i << "] expected:" << expected.begin_one.at(i) << " actual: " << actual.begin_one.at(i) << endl;
                return false;
            }
        }
    }

    for (int i = 0; i < expected.bit_arrays.size(); ++i) {
        vector<uint64_t> expected_bits, actual_bits;

        for (int j = 0; j < expected.bit_arrays.at(i).size(); ++j) {
            expected_bits.emplace_back(expected.bit_arrays.at(i).at(j));
        }
        for (int j = 0; j < actual.bit_arrays.at(i).size(); ++j) {
            actual_bits.emplace_back(actual.bit_arrays.at(i).at(j));
        }

        if (expected_bits != actual_bits) {
            return false;
        }
    }

    return true;
}

bool test_remove(uint64_t num, uint64_t num_of_alphabet) {
    vector<uint64_t> data(num);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(0, num_of_alphabet-1);
    for (int i = 0; i < num; ++i) {
        data[i] = alpha_distrib(gen);
    }

    dyn::wm_str actual(num_of_alphabet, data);

    for (int i = 0; i < num; ++i) {
        std::uniform_int_distribution<uint64_t> pos_distrib(0, actual.size()-1);
        uint64_t pos = pos_distrib(gen);

        data.erase(data.begin() + pos);

        dyn::wm_str expected(num_of_alphabet, data);
        actual.remove(pos);

        if (not same(expected, actual)) {
            return false;
        }
    }

    return true;
}

bool test_insert(uint64_t num, uint64_t num_of_alphabet) {

    dyn::wm_str actual(num_of_alphabet);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(0, num_of_alphabet-1);

    vector<uint64_t> data;
    for (int i = 0; i < num; ++i) {
        std::uniform_int_distribution<uint64_t> pos_distrib(0, actual.size()-1);
        uint64_t pos = pos_distrib(gen);
        uint64_t c = alpha_distrib(gen);

        data.insert(data.begin() + pos, c);

        dyn::wm_str expected(num_of_alphabet, data);

        actual.insert(pos, c);
        if (not same(expected, actual)) {
            return false;
        }
    }

    return true;
}

bool test_update(uint64_t num, uint64_t num_of_alphabet) {
    vector<uint64_t> data(num);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(0, num_of_alphabet-1);
    for (int i = 0; i < num; ++i) {
        data[i] = alpha_distrib(gen);
    }

    dyn::wm_str actual(num_of_alphabet, data);
    std::uniform_int_distribution<uint64_t> pos_distrib(0, actual.size()-1);

    for (int i = 0; i < num - 1; ++i) {
        uint64_t pos = pos_distrib(gen);
        uint64_t c = alpha_distrib(gen);
        data[pos] = c;

        dyn::wm_str expected(num_of_alphabet, data);
        actual.update(pos, c);

        if (not same(expected, actual)) {
            return false;
        }
    }

    return true;
}

bool test_serialize(uint64_t num, uint64_t num_of_alphabet) {
    vector<uint64_t> data(num);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(0, num_of_alphabet-1);
    for (int i = 0; i < num; ++i) {
        data[i] = alpha_distrib(gen);
    }

    dyn::wm_str expected(num_of_alphabet, data);
    string f = "wm_string.test.temp";
    ofstream out(f.c_str());
    expected.serialize(out);
    out.close();
    dyn::wm_str actual;
    ifstream in(f.c_str());
    actual.load(in);
    in.close();
    std::remove(f.c_str());

    if (not same(expected, actual)) {
        return false;
    }

    return true;
}

void speed_test(uint64_t num, uint64_t num_of_alphabet) {
    cout << "access:" << speed_access(num, num_of_alphabet) << "ms" << endl;
    cout << "rank:" << speed_rank(num, num_of_alphabet) << "ms" << endl;
    cout << "select:" << speed_select(num, num_of_alphabet) << "ms" << endl;
    cout << "insert:" << speed_insert(num, num_of_alphabet) << "ms" << endl;
    cout << "erase:" << speed_remove(num, num_of_alphabet) << "ms" << endl;
    cout << "update:" << speed_update(num, num_of_alphabet) << "ms" << endl;
}


bool test(uint64_t num, uint64_t num_of_alphabet) {
    bool ok = true;
    ok &= test_access(num, num_of_alphabet);
    ok &= test_rank(num, num_of_alphabet);
    ok &= test_select(num, num_of_alphabet);
    ok &= test_remove(num, num_of_alphabet);
    ok &= test_update(num, num_of_alphabet);
    ok &= test_serialize(num, num_of_alphabet);
    return ok;
}

int main() {

    uint64_t num = 100000;
    uint64_t num_of_alphabet = 10000000;

    cout << "SPEED alpha=100" << endl;
    speed_test(num, 100);
    cout << "SPEED alpha=" << num_of_alphabet << endl;
    speed_test(num, num_of_alphabet);

    cout << "TEST" << endl;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> alpha_distrib(1, 1000);
    std::uniform_int_distribution<uint64_t> len_distrib(1, 100);

    for (int i = 0; i < 100; ++i) {
        num = len_distrib(gen);
        num_of_alphabet = alpha_distrib(gen);
        cout << "TEST size=" << num << " alpha=" << num_of_alphabet << endl;
        if (not test(num, num_of_alphabet)) {
            cout << "ERROR!" << std::endl;
            break;
        }
    }
    cout << "OK" << endl;

    return 0;
}
