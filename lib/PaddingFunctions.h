/*
 * PaddingFunctions.h
 *
 *  Created on: Feb 18, 2010
 *      Author: saswat
 */

#ifndef PADDINGFUNCTIONS_H_
#define PADDINGFUNCTIONS_H_

class PaddingFunctions {
public:
	PaddingFunctions();
	virtual ~PaddingFunctions();
	int pad(String type);
	int unpad(String type);

};

#endif /* PADDINGFUNCTIONS_H_ */
