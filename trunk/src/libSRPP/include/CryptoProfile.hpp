/**
 * This class sets and uses the encryption profile supplied by the user.
 *
 * Saswat Mohanty <smohanty@cs.tamu.edu>
 */
#ifndef CRYPTO_PROFILE_H
#define CRYPTO_PROFILE_H

#include <string>

using namespace std;

class CryptoProfile {

	string encryption_algorithm;
	int BLOCK_SIZE ;

public:
	CryptoProfile() {}

	CryptoProfile(string algo){
		encryption_algorithm = algo;
		BLOCK_SIZE = 2;
	}

	~CryptoProfile(){}
};


#endif
