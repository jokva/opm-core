/*
  Will compare all the elements in the two double pointers p1 and
  p2. If all elements are 'sufficiently' the function will return 0,
  otherwise it will return 1.
*/

#ifndef MEMCMP_DOUBLE_H
#define MEMCMP_DOUBLE_H

#ifdef __cplusplus
extern "C" {
#endif


int memcmp_double(const double * p1 , const double *p2 , size_t num_elements);


#ifdef __cplusplus
}
#endif

#endif
