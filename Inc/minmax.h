/*
 * minmax.h
 *
 *  Created on: Apr 10, 2018
 *      Author: sid
 */

#ifndef VARIO_MISC_MINMAX_H_
#define VARIO_MISC_MINMAX_H_

#ifndef __cplusplus
#define max(a,b) \
		({ __typeof__ (a) _a = (a); \
		   __typeof__ (a) _b = (b); \
		   _a > _b ? _a : _b; })

#define min(a,b) \
		({ __typeof__ (a) _a = (a); \
		   __typeof__ (a) _b = (b); \
		   _a < _b ? _a : _b; })
#else
#define max(a, b)		std::max((a), (b))
#define min(a, b)		std::min((a), (b))
#endif

#endif /* VARIO_MISC_MINMAX_H_ */
