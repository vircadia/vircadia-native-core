//
//  field.h
//  interface
//
//  Created by Philip Rosedale on 8/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef interface_field_h
#define interface_field_h

//  Field is a lattice of vectors uniformly distributed FIELD_ELEMENTS^(1/3) on side 

const int FIELD_ELEMENTS = 1000;

void field_init();
int field_value(float *ret, float *pos);
void field_render();
void field_add(float* add, float *loc);

#endif
