#include "circle.h"

#include <stdio.h>
#include <stdlib.h>

static const double PI = 3.14159265358979323846;

typedef struct Circle {
    Shape base;
    double radius;
} Circle;

static double circle_area(const Shape *self) {
    const Circle *circle = (const Circle *)self;
    return PI * circle->radius * circle->radius;
}

static double circle_perimeter(const Shape *self) {
    const Circle *circle = (const Circle *)self;
    return 2.0 * PI * circle->radius;
}

static void circle_describe(const Shape *self) {
    const Circle *circle = (const Circle *)self;
    printf("%s(radius=%.2f) -> area=%.2f, perimeter=%.2f\n",
           shape_type_name(self),
           circle->radius,
           shape_area(self),
           shape_perimeter(self));
}

static void circle_destroy(Shape *self) {
    free(self);
}

static const ShapeOperations circle_operations = {
    circle_area,
    circle_perimeter,
    circle_describe,
    circle_destroy,
};

Shape *circle_create(double radius) {
    Circle *circle = malloc(sizeof(*circle));
    if (circle == NULL) {
        return NULL;
    }
    shape_init(&circle->base, &circle_operations, "Circle");
    circle->radius = radius;
    return &circle->base;
}
