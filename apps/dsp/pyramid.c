#include "../../drivers/vga/vgahandler.h"
#include "../../lib/flopmath.h"

#define PI 3.14159265

typedef struct {
    float x, y, z;
} Point3D;

typedef struct {
    int x, y;
} Point2D;

// Projection of 3D points to 2D screen coordinates
Point2D project(Point3D p, float fov, float viewer_distance) {
    Point2D projected;
    projected.x = (int)((fov * p.x) / (p.z + viewer_distance) + 160); // Centered on 320x200
    projected.y = (int)((fov * p.y) / (p.z + viewer_distance) + 100);
    return projected;
}

// Rotate a point around the Y axis
Point3D rotate_y(Point3D p, float angle) {
    Point3D rotated;
    rotated.x = p.x * cos(angle) - p.z * sin(angle);
    rotated.y = p.y;
    rotated.z = p.x * sin(angle) + p.z * cos(angle);
    return rotated;
}

void draw_pyramid(float angle) {
    // Define pyramid vertices
    Point3D vertices[5] = {
        { 0, 1, 0 },  // Apex
        { -1, -1, -1 }, { 1, -1, -1 }, { 1, -1, 1 }, { -1, -1, 1 } // Base
    };

    // Rotate and project vertices
    Point2D projected[5];
    for (int i = 0; i < 5; i++) {
        Point3D rotated = rotate_y(vertices[i], angle);
        projected[i] = project(rotated, 200, 5);
    }

    // Draw pyramid edges
    vga_draw_triangle(
        projected[0].x, projected[0].y, 
        projected[1].x, projected[1].y, 
        projected[2].x, projected[2].y, WHITE
    );
    vga_draw_triangle(
        projected[0].x, projected[0].y, 
        projected[2].x, projected[2].y, 
        projected[3].x, projected[3].y, WHITE
    );
    vga_draw_triangle(
        projected[0].x, projected[0].y, 
        projected[3].x, projected[3].y, 
        projected[4].x, projected[4].y, WHITE
    );
    vga_draw_triangle(
        projected[0].x, projected[0].y, 
        projected[4].x, projected[4].y, 
        projected[1].x, projected[1].y, WHITE
    );

    // Draw base
    vga_draw_line(projected[1].x, projected[1].y, projected[2].x, projected[2].y, WHITE);
    vga_draw_line(projected[2].x, projected[2].y, projected[3].x, projected[3].y, WHITE);
    vga_draw_line(projected[3].x, projected[3].y, projected[4].x, projected[4].y, WHITE);
    vga_draw_line(projected[4].x, projected[4].y, projected[1].x, projected[1].y, WHITE);
}
