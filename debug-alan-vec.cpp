#include "include/internal/packed_vector.hpp"
//#include "include/internal/packed_array.hpp"
#include <iostream>

using namespace std;
using namespace dyn;

int main() {
   // packed_vector v( 5, 8 );

   // v[0] = 18;
   // v[1] = 28;
   // v[2] = 32;
   // v[3] = 8;
   // v[4] = 120;

   packed_vector v;

   v.push_back(0);
   cout << v.bit_size() << endl;
   
   v.push_back(18);
   v.push_back(28);
   v.push_back(32);
   v.push_back(3);
   v.push_back(120);
   
   for (size_t i = 0; i < v.size(); ++i) {
      cout << i << ' ' << v[i] << endl;
   }

   cout << "bitsize: " << v.bit_size() << endl;

   cout << "sum: " << v.psum() << endl;
   cout << "psum(3): " << v.psum( 3 ) << endl;
   cout << "contains( psum(3) ): " << v.contains(v.psum( 3 )) << endl;
   cout << "contains( psum(3) + 1 ): " << v.contains(v.psum( 3 ) + 1) << endl;
   cout << "search( psum(3) + 1 ): " << v.search(v.psum( 3 ) + 1) << endl;
   cout << "search( psum(3) - 1 ): " << v.search(v.psum( 3 ) - 1) << endl;
   cout << "insert( 2, 19 ) " << endl;
   v.insert(2,19);
   

   for (size_t i = 0; i < v.size(); ++i) {
      cout << i << ' ' << v[i] << endl;
   }

   cout << "sum: " << v.psum() << endl;
   
   cout << "Hello, world!" << endl;

   return 0;
}
