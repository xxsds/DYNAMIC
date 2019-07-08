#include <iostream>
#include <chrono>
#include <cstdlib>
#include <dynamic.hpp>

using namespace std;
using namespace dyn;

int main() {

   dyn::succinct_spsi spsi;
   dyn::spsi_check<> check;

   int n=40000;
   int sigma=120;

   srand(time(NULL));

   for(int i=0;i<n;++i){

	   cout << i << endl;

	   int j = rand()%(i+1);
	   int x = rand()%sigma;

	   spsi.insert(j,x);
	   check.insert(j,x);

   }

   for(int i=0;i<n;++i){

 	   assert(spsi[i]==check[i]);

    }


   return 0;
}
