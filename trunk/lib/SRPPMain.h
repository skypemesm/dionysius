/*
 * SRPPMain.h
 *
 *  Created on: Feb 18, 2010
 *      Author: saswat
 */

#ifndef SRPPMAIN_H_
#define SRPPMAIN_H_

class SRPPMain {
public:
	SRPPMain();
	virtual ~SRPPMain();
	int srpp_init();
	int srpp_start_session(SRPPSession thissession);
};

#endif /* SRPPMAIN_H_ */
