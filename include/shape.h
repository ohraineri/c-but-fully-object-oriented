#ifndef SHAPE_H
#define SHAPE_H

typedef struct Shape Shape;

typedef struct ShapeOperations {
    double (*area)(const Shape *self);
    double (*perimeter)(const Shape *self);
    void (*describe)(const Shape *self);
    void (*destroy)(Shape *self);
} ShapeOperations;

struct Shape {
    const ShapeOperations *operations;
    const char *type_name;
};

void shape_init(Shape *self, const ShapeOperations *operations, const char *type_name);

double shape_area(const Shape *self);
double shape_perimeter(const Shape *self);
void shape_describe(const Shape *self);
void shape_destroy(Shape *self);

const char *shape_type_name(const Shape *self);

#endif
