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
   // packed_vector v( 5, 8 );

   // v[0] = 18;
   // v[1] = 28;
   // v[2] = 32;
   // v[3] = 8;
   // v[4] = 120;

   //   packed_vector v;
   packed_spsi sp;
   
   //   v.push_back(0);
   sp.insert( 0, 0 );

   //   v.push_back(18);
   sp.insert(1, 18);
   //   v.push_back(28);
   sp.insert(2, 28);
   //   v.push_back(32);
   sp.insert(3, 32);
   //   v.push_back(3);
   sp.insert(4, 3);
   //   v.push_back(120);
   sp.insert(5,120);
   
   for (size_t i = 0; i < sp.size(); ++i) {
      cout << i << ' ' << sp[i] << endl;
   }

   //   cout << "bitsize (vec): " << v.bit_size() << endl;
   cout << "bitsize (spsi): " << sp.bit_size() << endl;
   
   //cout << "sum (vec): " << v.psum() << endl;
   cout << "sum (spsi): " << sp.psum() << endl;
   // cout << "psum(3): " << v.psum( 3 ) << endl;
   // cout << "contains( psum(3) ): " << v.contains(v.psum( 3 )) << endl;
   // cout << "contains( psum(3) + 1 ): " << v.contains(v.psum( 3 ) + 1) << endl;
   // cout << "search( psum(3) + 1 ): " << v.search(v.psum( 3 ) + 1) << endl;
   // cout << "search( psum(3) - 1 ): " << v.search(v.psum( 3 ) - 1) << endl;
   // cout << "insert( 2, 19 ) " << endl;
   // v.insert(2,19);
   

   // for (size_t i = 0; i < v.size(); ++i) {
   //    cout << i << ' ' << v[i] << endl;
   // }

   // cout << "sum: " << v.psum() << endl;

   //  cout << "remove( 3 ) " << endl;
   //  v.remove(3);
   

   // for (size_t i = 0; i < v.size(); ++i) {
   //    cout << i << ' ' << v[i] << endl;
   // }

   // cout << "sum: " << v.psum() << endl;
   

   size_t n_ops = 10000000;
   srand( time( NULL ) );
   using std::chrono::high_resolution_clock;
   using std::chrono::duration_cast;
   using std::chrono::duration;

   //   vector< uint64_t > positions;
   //vector< uint64_t > vals;
   size_t sum = 201;
   // for (size_t i = 0; i < n_ops; ++i) {
   //    //random insertion
   //    size_t pos = rand() % ( sp.size() + 1 );

   //    positions.push_back( pos );
   //    vals.push_back( rand() );
   //    sum += vals.back();
   // }

   //cout << "insert (vec)... " << flush;
   auto t1 = high_resolution_clock::now();
   for (size_t i = 0; i < n_ops; ++i) {
      //random insertion
      //v.insert( positions[i], vals[i] );
   }
   auto t2 = high_resolution_clock::now();
   //cout << "done." << endl;
   cout << "insert (spsi) ... " << flush;
   auto t5 = high_resolution_clock::now();
   for (size_t i = 0; i < n_ops; ++i) {
      //random insertion
      size_t pos = rand() % ( sp.size() + 1 );
      
      sp.insert( pos, rand() );
   }
   auto t6 = high_resolution_clock::now();
   cout << "done." << endl;

   //   cout << "bitsize (vec): " << v.bit_size() << endl;
   cout << "Size of spsi: " << sp.size() << endl;
   cout << "bitsize (spsi): " << sp.bit_size() << endl;
   
   //cout << "checking access..." << flush;
   //   auto t5 = high_resolution_clock::now();
   // for (size_t i = 0; i < n_ops; ++i) {
   //    //random insertion
   //    size_t pos = rand() % (v.size());
   //    if ( sp[pos] != v[ pos ] ) {
   // 	 cout << "access failed" << endl;
   // 	 return 1;
   //    }
   // }
   //auto t6 = high_resolution_clock::now();
   //cout << "skipped" << endl;

   //   cout << "sum (vec): " << v.psum() << endl;
   cout << "sum (spsi): " << sp.psum() << endl;
   cout << "sum of inserted elements: " << sum << endl;
   
   cout << "remove (vec) ... " << flush;
   auto t3 = high_resolution_clock::now();
   for (size_t i = 0; i < n_ops; ++i) {
      //random insertion
      //v.remove( positions[ n_ops - i - 1 ] );
   }

   auto t4 = high_resolution_clock::now();
   cout << "done." << endl;

   cout << "remove (spsi) ... " << flush;
   auto t7 = high_resolution_clock::now();
   for (size_t i = 0; i < n_ops; ++i) {
      //random insertion
      size_t pos = rand() % ( sp.size() );
      sp.remove( pos );
   }

   auto t8 = high_resolution_clock::now();
   cout << "done." << endl;

   uint64_t sec_insert = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
   uint64_t sec_remove = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
   uint64_t sec_spsi_insert = std::chrono::duration_cast<std::chrono::microseconds>(t6 - t5).count();
   uint64_t sec_spsi_remove = std::chrono::duration_cast<std::chrono::microseconds>(t8 - t7).count();

   //   cout << (double)sec_insert / n_ops << " microseconds/insert (vec)" << endl;
   cout << (double)sec_spsi_insert / n_ops << " microseconds/insert (spsi)" << endl;
   //cout << (double)sec_remove / n_ops << " microseconds/remove (vec)" << endl;
   cout << (double)sec_spsi_remove / n_ops << " microseconds/remove (spsi)" << endl;

   cout << "Size of spsi: " << sp.size() << endl;
   cout << "Bitsize of spsi: " << sp.bit_size() << endl;

   for (size_t i = 0; i < sp.size(); ++i) {
      cout << i << ' ' << sp[ i] << endl;
   }

   cout << "sum (spsi): " << sp.psum() << endl;

   while (sp.size() > 0) {
      sp.remove( sp.size() - 1);
   }

   cout << "sum (spsi): " << sp.psum() << endl;
   
   return 0;
}
