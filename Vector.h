/* Do not edit this file. It was automatically generated. */

#ifndef HEADER_Vector
#define HEADER_Vector
/*
htop
(C) 2004-2006 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "Object.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "debug.h"
#include <assert.h>


#ifndef DEFAULT_SIZE
#define DEFAULT_SIZE -1
#endif

typedef void(*Vector_procedure)(void*);

typedef struct Vector_ {
   Object **array;
   Object_Compare compare;
   int arraySize;
   int growthRate;
   int items;
   char* vectorType;
   bool owner;
} Vector;


Vector* Vector_new(char* vectorType_, bool owner, int size, Object_Compare compare);

void Vector_delete(Vector* this);

#ifdef DEBUG

int Vector_count(Vector* this);

#endif

void Vector_prune(Vector* this);

void Vector_sort(Vector* this);

void Vector_insert(Vector* this, int index, void* data_);

Object* Vector_take(Vector* this, int index);

Object* Vector_remove(Vector* this, int index);

void Vector_moveUp(Vector* this, int index);

void Vector_moveDown(Vector* this, int index);

void Vector_set(Vector* this, int index, void* data_);

extern Object* Vector_get(Vector* this, int index);

extern int Vector_size(Vector* this);

/*

*/

void Vector_add(Vector* this, void* data_);

extern int Vector_indexOf(Vector* this, void* search_, Object_Compare compare);

/*

*/

#endif
