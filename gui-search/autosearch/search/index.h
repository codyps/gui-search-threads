#ifndef INDEX_H_
#define INDEX_H_

#include <stdlib.h>
#include "sorted-list.h"
#include "util.h"

keyword_t **index_read(size_t *n, FILE *stream);

#endif
