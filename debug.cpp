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

   dyn::succinct_spsi spsi;
   spsi.insert(0,0);
   spsi.remove(0);
   spsi.insert(0,0);
   spsi.insert(0,0);
   spsi.insert(2,1);
   spsi.insert(1,1);
   spsi.remove(3);

   for(size_t i = 0; i < spsi.size(); ++i)
      std::cout << spsi[i] << ",";
   std::cout << std::endl;

   spsi.insert(3,1);

   for(size_t i = 0; i < spsi.size(); ++i)
      std::cout << spsi[i] << ",";
   std::cout << std::endl;

   return 0;
}
