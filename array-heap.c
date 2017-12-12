#include <stdlib.h>
#include <stdio.h>

#include "array-heap.h"

int array_init(array* arr, int size) {
  arr->data = reallloc(NULL, sizeof(void*) * size);
  
  if ( (size_t)arr->data == 0 )
    return -1;
    
  arr->length = size;
  arr->index = 0;
  
  return 0;
}
