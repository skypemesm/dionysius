/**
 * This class sets and uses the encryption profile supplied by the user.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */

#include <string>

using namespace std;

class CryptoProfile {

	string encryption_algorithm;
	int BLOCK_SIZE ;

public:
	CryptoProfile(string algo){
		encryption_algorithm = algo;
		BLOCK_SIZE = 2;
	}

	~CryptoProfile(){

	}
};
