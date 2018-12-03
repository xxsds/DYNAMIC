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
   string b = "banana";
   wt_str st( 26, b );

   for (size_t i = 0; i < st.size(); ++i) {
      cout << static_cast<char>(st[i]);
   }
   cout << endl;
   st.remove(0);
   
   for (size_t i = 0; i < st.size(); ++i) {
      cout << static_cast<char>(st[i]);
   }
   cout << endl;

   st.remove(2);

   for (size_t i = 0; i < st.size(); ++i) {
      cout << static_cast<char>(st[i]);
   }
   cout << endl;
   return 0;
}
