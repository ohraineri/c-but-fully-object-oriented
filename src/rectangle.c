#include "rectangle.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct Rectangle {
    Shape base;
    double width;
    double height;
} Rectangle;

static double rectangle_area(const Shape *self) {
    const Rectangle *rectangle = (const Rectangle *)self;
    return rectangle->width * rectangle->height;
}

static double rectangle_perimeter(const Shape *self) {
    const Rectangle *rectangle = (const Rectangle *)self;
    return 2.0 * (rectangle->width + rectangle->height);
}

static void rectangle_describe(const Shape *self) {
    const Rectangle *rectangle = (const Rectangle *)self;
    printf("%s(width=%.2f, height=%.2f) -> area=%.2f, perimeter=%.2f\n",
           shape_type_name(self),
           rectangle->width,
           rectangle->height,
           shape_area(self),
           shape_perimeter(self));
}

static void rectangle_destroy(Shape *self) {
    free(self);
}

static const ShapeOperations rectangle_operations = {
    rectangle_area,
    rectangle_perimeter,
    rectangle_describe,
    rectangle_destroy,
};

Shape *rectangle_create(double width, double height) {
    Rectangle *rectangle = malloc(sizeof(*rectangle));
    if (rectangle == NULL) {
        return NULL;
    }
    shape_init(&rectangle->base, &rectangle_operations, "Rectangle");
    rectangle->width = width;
    rectangle->height = height;
    return &rectangle->base;
}
