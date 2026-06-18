#include <stdio.h>

#include "circle.h"
#include "rectangle.h"
#include "shape_collection.h"

int main(void) {
    ShapeCollection *shapes = shape_collection_create();
    if (shapes == NULL) {
        fprintf(stderr, "Failed to allocate the shape collection\n");
        return 1;
    }

    shape_collection_add(shapes, circle_create(2.0));
    shape_collection_add(shapes, rectangle_create(3.0, 4.0));
    shape_collection_add(shapes, circle_create(1.5));
    shape_collection_add(shapes, rectangle_create(5.0, 5.0));

    printf("Collection holds %zu shapes:\n", shape_collection_size(shapes));
    shape_collection_describe_all(shapes);
    printf("Total area: %.2f\n", shape_collection_total_area(shapes));

    shape_collection_destroy(shapes);
    return 0;
}
