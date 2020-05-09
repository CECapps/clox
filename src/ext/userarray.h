#ifndef cc_ext_array_h
#define cc_ext_array_h

#include "../object.h"

void ua_grow(ObjUserArray* ua, int new_capacity);
void cc_register_ext_userarray();

#endif
