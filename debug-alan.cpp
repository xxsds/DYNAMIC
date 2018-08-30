#include "include/internal/packed_vector.hpp"
//#include "inlcude/internal/spsi.hpp"
#include <dynamic.hpp>
//#include "include/internal/packed_array.hpp"
#include <iostream>
#include <chrono>
#include <cstdlib>

using namespace std;
using namespace dyn;

int main() {

   gap_bv bv;
   size_t ops = 10;
   for (size_t i = 0; i < ops; ++i){
      bv.insert( rand() % bv.size() + 1, rand() % 2 );
   }

   for (size_t i = 0; i < bv.size(); ++i) {
      cout << bv[i];
   }
   cout << endl;

   
   
   return 0;
}
