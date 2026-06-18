#include "shape_collection.h"

#include <stdio.h>
#include <stdlib.h>

static const size_t INITIAL_CAPACITY = 4;

struct ShapeCollection {
    Shape **items;
    size_t size;
    size_t capacity;
};

static int shape_collection_grow(ShapeCollection *self) {
    size_t new_capacity = self->capacity == 0 ? INITIAL_CAPACITY : self->capacity * 2;
    Shape **resized = realloc(self->items, new_capacity * sizeof(*resized));
    if (resized == NULL) {
        return 0;
    }
    self->items = resized;
    self->capacity = new_capacity;
    return 1;
}

ShapeCollection *shape_collection_create(void) {
    ShapeCollection *self = malloc(sizeof(*self));
    if (self == NULL) {
        return NULL;
    }
    self->items = NULL;
    self->size = 0;
    self->capacity = 0;
    return self;
}

int shape_collection_add(ShapeCollection *self, Shape *shape) {
    if (self == NULL || shape == NULL) {
        return 0;
    }
    if (self->size == self->capacity && !shape_collection_grow(self)) {
        return 0;
    }
    self->items[self->size] = shape;
    self->size++;
    return 1;
}

size_t shape_collection_size(const ShapeCollection *self) {
    return self->size;
}

Shape *shape_collection_at(const ShapeCollection *self, size_t index) {
    if (index >= self->size) {
        return NULL;
    }
    return self->items[index];
}

double shape_collection_total_area(const ShapeCollection *self) {
    double total = 0.0;
    for (size_t index = 0; index < self->size; index++) {
        total += shape_area(self->items[index]);
    }
    return total;
}

void shape_collection_describe_all(const ShapeCollection *self) {
    for (size_t index = 0; index < self->size; index++) {
        printf("[%zu] ", index);
        shape_describe(self->items[index]);
    }
}

void shape_collection_destroy(ShapeCollection *self) {
    if (self == NULL) {
        return;
    }
    for (size_t index = 0; index < self->size; index++) {
        shape_destroy(self->items[index]);
    }
    free(self->items);
    free(self);
}
