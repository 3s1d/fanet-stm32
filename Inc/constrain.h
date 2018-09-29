/*
 * constrain.h
 *
 *  Created on: Jul 5, 2018
 *      Author: sid
 */

#ifndef CONSTRAIN_H_
#define CONSTRAIN_H_


template<class T>
const T& constrain(const T& x, const T& lo, const T& hi)
{
	if(x < lo)
		return lo;
	else if(hi < x)
		return hi;
	else
		return x;
}


#endif /* CONSTRAIN_H_ */
