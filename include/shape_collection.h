#ifndef SHAPE_COLLECTION_H
#define SHAPE_COLLECTION_H

#include <stddef.h>

#include "shape.h"

typedef struct ShapeCollection ShapeCollection;

ShapeCollection *shape_collection_create(void);

int shape_collection_add(ShapeCollection *self, Shape *shape);

size_t shape_collection_size(const ShapeCollection *self);

Shape *shape_collection_at(const ShapeCollection *self, size_t index);

double shape_collection_total_area(const ShapeCollection *self);

void shape_collection_describe_all(const ShapeCollection *self);

void shape_collection_destroy(ShapeCollection *self);

#endif
