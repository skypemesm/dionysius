/***
 *  \file Padding_algorithms.cpp
 *
 *  This file implements the algorithms needed for different padding techniques
 *
 *  ADD YOUR NEW PADDING ALGORITHMS HERE...
 *
 *  Saswat Mohanty <smohanty@cs.tamu.edu>
 *
 */

#include <string>

using namespace std;

class PaddingAlgos {

public:
	int psp_algo(string algo);
	int cbp_algo(string algo);
	int ebp_algo(string algo);
	int vitp_algo(string algo);



private:


	int default_psp_algo(string algo);
	int default_cbp_algo(string algo);
	int default_ebp_algo(string algo);
	int default_vitp_algo(string algo);


};
