#include "shape.h"

#include <stddef.h>

void shape_init(Shape *self, const ShapeOperations *operations, const char *type_name) {
    self->operations = operations;
    self->type_name = type_name;
}

double shape_area(const Shape *self) {
    return self->operations->area(self);
}

double shape_perimeter(const Shape *self) {
    return self->operations->perimeter(self);
}

void shape_describe(const Shape *self) {
    self->operations->describe(self);
}

void shape_destroy(Shape *self) {
    if (self == NULL) {
        return;
    }
    self->operations->destroy(self);
}

const char *shape_type_name(const Shape *self) {
    return self->type_name;
}
