#ifndef IA_TIME_H_
#define IA_TIME_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

typedef uintmax_t ia_timestamp_t;
ia_timestamp_t ia_timestamp_ns(void);

#define US 1000ull
#define MS 1000000ull
#define S  1000000000ull

#endif /* IA_TIME_H_ */
