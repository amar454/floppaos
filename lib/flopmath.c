/*
flopmath.c - mathematical functions for floppaOS

Copyright 2024 Amar Djulovic <aaamargml@gmail.com>

This file is part of FloppaOS.

FloppaOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FloppaOS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with FloppaOS. If not, see <https://www.gnu.org/licenses/>.
*/

#include "flopmath.h"

// Factorial function for Taylor series
double factorial(int n) {
    if (n < 0) return NAN; // Factorial is not defined for negative numbers
    double result = 1.0;
    for (int i = 1; i <= n; i++) {
        result *= i;
    }
    return result;
}

// Power function (base^exp)
double power(double base, int exp) {
    if (exp == 0) return 1.0;
    double result = 1.0;
    int positive_exp = exp < 0 ? -exp : exp;
    for (int i = 0; i < positive_exp; i++) {
        result *= base;
    }
    return exp < 0 ? 1.0 / result : result;
}

// Exponentiation function (e^x)
double exp(double x) {
    double term = 1.0; // Current term in series
    double sum = 1.0;  // Initialize with the first term
    for (int n = 1; n <= 20; n++) {
        term *= x / n;
        sum += term;
    }
    return sum;
}

// Natural logarithm (ln(x))
double ln(double x) {
    if (x <= 0) return NAN; // Log is undefined for x <= 0
    double sum = 0.0;
    double term = (x - 1) / (x + 1);
    double term_squared = term * term;
    for (int n = 1; n < 50; n += 2) {
        sum += (1.0 / n) * power(term, n);
    }
    return 2 * sum;
}

// Adjust angle to range [0, 2*PI]
double normalize_angle(double x) {
    while (x < 0) x += 2 * PI;
    while (x >= 2 * PI) x -= 2 * PI;
    return x;
}

// Sine function using Taylor series
double sin(double x) {
    x = normalize_angle(x);
    double term = x;
    double sum = x;
    for (int n = 1; n <= 10; n++) {
        term *= -1 * x * x / ((2 * n) * (2 * n + 1));
        sum += term;
    }
    return sum;
}

// Cosine function using Taylor series
double cos(double x) {
    x = normalize_angle(x);
    double term = 1.0;
    double sum = 1.0;
    for (int n = 1; n <= 10; n++) {
        term *= -1 * x * x / ((2 * n - 1) * (2 * n));
        sum += term;
    }
    return sum;
}

// Tangent function using sine and cosine
double tan(double x) {
    double s = sin(x);
    double c = cos(x);
    return (c == 0) ? NAN : s / c;
}

// Absolute value for floating-point numbers
double fabs(double x) {
    return x < 0 ? -x : x;
}

// Absolute value for integers
int abs(int x) {
    return x < 0 ? -x : x;
}

// Square root function using Newton's method
double sqrt(double x) {
    if (x < 0) return NAN;
    double guess = x / 2.0, prev;
    do {
        prev = guess;
        guess = (guess + x / guess) / 2.0;
    } while (fabs(guess - prev) > 1e-9);
    return guess;
}

// Logarithm base 10 function using natural logarithm
double log10(double x) {
    return ln(x) / ln(10.0);
}

// Power function (x^y) using the relationship a^b = e^(b*ln(a))
double pow(double base, double exp) {
    if (base <= 0) return NAN; // Undefined for base <= 0
    return exp * ln(base);
}

// Hyperbolic sine (sinh) function using Taylor series
double sinh(double x) {
    double term = x, sum = x;
    for (int n = 1; n <= 10; n++) {
        term *= x * x / ((2 * n) * (2 * n + 1));
        sum += term;
    }
    return sum;
}

// Hyperbolic cosine (cosh) function using Taylor series
double cosh(double x) {
    double term = 1.0, sum = 1.0;
    for (int n = 1; n <= 10; n++) {
        term *= x * x / ((2 * n - 1) * (2 * n));
        sum += term;
    }
    return sum;
}

// Hyperbolic tangent (tanh) function using sinh and cosh
double tanh(double x) {
    double sh = sinh(x), ch = cosh(x);
    return sh / ch;
}

// Arcsine function (asin) using Newton's method approximation
double asin(double x) {
    if (x < -1 || x > 1) return NAN; // Undefined for |x| > 1
    double guess = x, prev;
    do {
        prev = guess;
        guess = prev - (sin(prev) - x) / cos(prev);
    } while (fabs(guess - prev) > 1e-9);
    return guess;
}

// Arccosine function (acos) using numerical method
double acos(double x) {
    if (x < -1 || x > 1) return NAN; // Undefined for |x| > 1
    return PI / 2 - asin(x);
}

// Arctangent function (atan) using numerical method
double atan(double x) {
    double sum = 0.0, term = x;
    for (int n = 0; n < 10; n++) {
        sum += (n % 2 == 0 ? 1 : -1) * term / (2 * n + 1);
        term *= x * x;
    }
    return sum;
}

// Arctangent of y/x (atan2) using atan
double atan2(double y, double x) {
    if (x > 0) return atan(y / x);
    if (x < 0 && y >= 0) return atan(y / x) + PI;
    if (x < 0 && y < 0) return atan(y / x) - PI;
    if (x == 0 && y > 0) return PI / 2;
    if (x == 0 && y < 0) return -PI / 2;
    return NAN; // Undefined for x = 0 and y = 0
}

// Rounding function (round)
double round(double x) {
    return (x >= 0) ? floor(x + 0.5) : ceil(x - 0.5);
}

// Floor function (largest integer less than or equal to x)
double floor(double x) {
    int xi = (int)x;
    return (x < xi) ? xi - 1 : xi;
}

// Ceil function (smallest integer greater than or equal to x)
double ceil(double x) {
    int xi = (int)x;
    return (x > xi) ? xi + 1 : xi;
}

// Min function
double min(double a, double b) {
    return (a < b) ? a : b;
}

// Max function
double max(double a, double b) {
    return (a > b) ? a : b;
}

// Absolute value function (for integer types)
int abs_int(int x) {
    return x < 0 ? -x : x;
}

// Logarithm base 2 (log2)
double log2(double x) {
    return ln(x) / ln(2.0);
}

// Cube root function (cbrt)
double cbrt(double x) {
    if (x < 0) return -pow(-x, 1.0/3.0);
    return pow(x, 1.0/3.0);
}

// Greatest common divisor (gcd) of two integers
int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// Least common multiple (lcm) of two integers
int lcm(int a, int b) {
    return (a * b) / gcd(a, b);
}

// Hyperbolic secant (sech)
double sech(double x) {
    return 1.0 / cosh(x);
}

// Hyperbolic cosecant (csch)
double csch(double x) {
    return 1.0 / sinh(x);
}

// Hyperbolic cotangent (coth)
double coth(double x) {
    return cosh(x) / sinh(x);
}

// Computes the sine of an angle in degrees (uses sin function for radians)
double sind(double x) {
    return sin(x * (PI / 180.0)); // Convert degrees to radians
}

// Computes the cosine of an angle in degrees (uses cos function for radians)
double cosd(double x) {
    return cos(x * (PI / 180.0)); // Convert degrees to radians
}

// Computes the tangent of an angle in degrees (uses tan function for radians)
double tand(double x) {
    return tan(x * (PI / 180.0)); // Convert degrees to radians
}

// Computes the cotangent of an angle (1/tan(x))
double cot(double x) {
    double t = tan(x);
    return (t == 0) ? NAN : 1.0 / t;
}

// Computes the secant of an angle (1/cos(x))
double sec(double x) {
    double c = cos(x);
    return (c == 0) ? NAN : 1.0 / c;
}

// Computes the cosecant of an angle (1/sin(x))
double csc(double x) {
    double s = sin(x);
    return (s == 0) ? NAN : 1.0 / s;
}

// Computes the exponential of a base raised to the power of an exponent (a^b)
double exp_base(double a, double b) {
    return power(a, (int)b);  // Use previously defined power function
}

// Finds the nth Fibonacci number using an iterative approach
long long fib(int n) {
    if (n < 0) return -1; // Invalid input
    long long a = 0, b = 1, temp;
    for (int i = 0; i < n; i++) {
        temp = a;
        a = b;
        b = temp + b;
    }
    return a;
}


// Computes the mean of an array of numbers
double mean(double arr[], int size) {
    if (size == 0) return NAN;
    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum / size;
}

// Computes the variance of an array of numbers
double variance(double arr[], int size) {
    double m = mean(arr, size);
    double var = 0.0;
    for (int i = 0; i < size; i++) {
        var += (arr[i] - m) * (arr[i] - m);
    }
    return var / size;
}

// Computes the standard deviation of an array of numbers
double stddev(double arr[], int size) {
    return sqrt(variance(arr, size));
}

// Computes the factorial of a number (int) recursively
long long factorial_recursive(int n) {
    if (n < 0) return -1; // Undefined for negative numbers
    if (n == 0) return 1;
    return n * factorial_recursive(n - 1);
}


// Vector operations
Vector3 vector_add(Vector3 a, Vector3 b) {
    return (Vector3){a.x + b.x, a.y + b.y, a.z + b.z};
}

Vector3 vector_subtract(Vector3 a, Vector3 b) {
    return (Vector3){a.x - b.x, a.y - b.y, a.z - b.z};
}

Vector3 vector_scalar_multiply(Vector3 v, double scalar) {
    return (Vector3){v.x * scalar, v.y * scalar, v.z * scalar};
}

double vector_dot(Vector3 a, Vector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 vector_cross(Vector3 a, Vector3 b) {
    return (Vector3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

double vector_magnitude(Vector3 v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector3 vector_normalize(Vector3 v) {
    double mag = vector_magnitude(v);
    return (mag > 0) ? vector_scalar_multiply(v, 1.0 / mag) : v;
}

double vector_angle_between(Vector3 a, Vector3 b) {
    double dot = vector_dot(a, b);
    double mag_a = vector_magnitude(a);
    double mag_b = vector_magnitude(b);
    return acos(dot / (mag_a * mag_b));
}

Vector3 vector_project(Vector3 a, Vector3 b) {
    double dot = vector_dot(a, b);
    double mag_b_squared = vector_dot(b, b);
    return vector_scalar_multiply(b, dot / mag_b_squared);
}

// Identity matrix
Matrix4x4 matrix_identity() {
    Matrix4x4 result = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
    return result;
}

// Matrix multiplication
Matrix4x4 matrix_multiply(Matrix4x4 a, Matrix4x4 b) {
    Matrix4x4 result = {{{0}}};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }
    return result;
}


// Matrix translation
Matrix4x4 matrix_translation(double tx, double ty, double tz) {
    Matrix4x4 result = matrix_identity();
    result.m[3][0] = tx;
    result.m[3][1] = ty;
    result.m[3][2] = tz;
    return result;
}

// Matrix scaling
Matrix4x4 matrix_scaling(double sx, double sy, double sz) {
    Matrix4x4 result = matrix_identity();
    result.m[0][0] = sx;
    result.m[1][1] = sy;
    result.m[2][2] = sz;
    return result;
}

// Matrix rotation around X-axis
Matrix4x4 matrix_rotate_x(double angle) {
    Matrix4x4 result = matrix_identity();
    result.m[1][1] = cos(angle);
    result.m[1][2] = -sin(angle);
    result.m[2][1] = sin(angle);
    result.m[2][2] = cos(angle);
    return result;
}

// Matrix rotation around Y-axis
Matrix4x4 matrix_rotate_y(double angle) {
    Matrix4x4 result = matrix_identity();
    result.m[0][0] = cos(angle);
    result.m[0][2] = sin(angle);
    result.m[2][0] = -sin(angle);
    result.m[2][2] = cos(angle);
    return result;
}

// Matrix rotation around Z-axis
Matrix4x4 matrix_rotate_z(double angle) {
    Matrix4x4 result = matrix_identity();
    result.m[0][0] = cos(angle);
    result.m[0][1] = -sin(angle);
    result.m[1][0] = sin(angle);
    result.m[1][1] = cos(angle);
    return result;
}

// Perspective projection matrix
Matrix4x4 matrix_perspective(double fov, double aspect, double near, double far) {
    double tan_half_fov = tan(fov / 2.0);
    Matrix4x4 result = {{{0}}};
    result.m[0][0] = 1.0 / (aspect * tan_half_fov);
    result.m[1][1] = 1.0 / tan_half_fov;
    result.m[2][2] = -(far + near) / (far - near);
    result.m[2][3] = -1.0;
    result.m[3][2] = -(2.0 * far * near) / (far - near);
    return result;
}

// LookAt view matrix
Matrix4x4 matrix_look_at(Vector3 eye, Vector3 target, Vector3 up) {
    Vector3 zaxis = vector_normalize(vector_subtract(eye, target)); // Camera forward
    Vector3 xaxis = vector_normalize(vector_cross(up, zaxis)); // Right
    Vector3 yaxis = vector_cross(zaxis, xaxis); // Up

    Matrix4x4 result = matrix_identity();
    result.m[0][0] = xaxis.x;
    result.m[0][1] = xaxis.y;
    result.m[0][2] = xaxis.z;
    result.m[1][0] = yaxis.x;
    result.m[1][1] = yaxis.y;
    result.m[1][2] = yaxis.z;
    result.m[2][0] = -zaxis.x;
    result.m[2][1] = -zaxis.y;
    result.m[2][2] = -zaxis.z;
    result.m[3][0] = -vector_dot(xaxis, eye);
    result.m[3][1] = -vector_dot(yaxis, eye);
    result.m[3][2] = vector_dot(zaxis, eye);
    return result;
}


// Smoothstep (easing function)
double smoothstep(double edge0, double edge1, double x) {
    double t = (x - edge0) / (edge1 - edge0);
    t = (t < 0) ? 0 : (t > 1) ? 1 : t;
    return t * t * (3.0 - 2.0 * t);
}

// Quadratic equation solver: ax^2 + bx + c = 0
int solve_quadratic(double a, double b, double c, double *root1, double *root2) {
    if (a == 0) {
        if (b == 0) return 0; // No solution
        *root1 = -c / b;
        return 1; // One solution
    }
    double discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return 0; // No real roots
    *root1 = (-b + sqrt(discriminant)) / (2 * a);
    *root2 = (-b - sqrt(discriminant)) / (2 * a);
    return (discriminant == 0) ? 1 : 2; // Return number of roots
}

// Linear interpolation for a vector (lerp)
Vector3 vector_lerp(Vector3 a, Vector3 b, double t) {
    return vector_add(vector_scalar_multiply(a, 1 - t), vector_scalar_multiply(b, t));
}


// Matrix determinant (2x2 matrix)
double determinant_2x2(double a, double b, double c, double d) {
    return a * d - b * c;
}

// Matrix determinant (3x3 matrix)
double determinant_3x3(double m[3][3]) {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
           m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
           m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

double compute_lighting(Vector3 normal, Vector3 light_dir) {
    normal = vector_normalize(normal);
    light_dir = vector_normalize(light_dir);
    double intensity = vector_dot(normal, light_dir);
    return max(0.0, intensity); // Clamp to [0, 1]
}
int solve_linear_system(int n, double matrix[][n + 1], double results[]) {
    for (int i = 0; i < n; i++) {
        // Find the pivot row
        int pivotRow = i;
        for (int k = i + 1; k < n; k++) {
            if (fabs(matrix[k][i]) > fabs(matrix[pivotRow][i])) {
                pivotRow = k;
            }
        }

        // Swap rows if needed
        if (pivotRow != i) {
            for (int j = 0; j <= n; j++) {
                double temp = matrix[i][j];
                matrix[i][j] = matrix[pivotRow][j];
                matrix[pivotRow][j] = temp;
            }
        }

        // Check for singularity
        double diag = matrix[i][i];
        if (fabs(diag) < 1e-9) return 0; // Singular matrix

        // Normalize the pivot row
        for (int j = 0; j <= n; j++) {
            matrix[i][j] /= diag;
        }

        // Eliminate all other rows
        for (int k = 0; k < n; k++) {
            if (k != i) {
                double factor = matrix[k][i];
                for (int j = 0; j <= n; j++) {
                    matrix[k][j] -= factor * matrix[i][j];
                }
            }
        }
    }

    // Extract results
    for (int i = 0; i < n; i++) {
        results[i] = matrix[i][n];
    }
    return 1; // Solution exists
}

// Polynomial evaluation using Horner's Method
double evaluate_polynomial(double coefficients[], int degree, double x) {
    double result = coefficients[0];
    for (int i = 1; i <= degree; i++) {
        result = result * x + coefficients[i];
    }
    return result;
}

// Numerical differentiation (central difference method)
double numerical_differentiation(double (*f)(double), double x, double h) {
    return (f(x + h) - f(x - h)) / (2 * h);
}

// Trapezoidal rule
double trapezoidal_integration(double (*f)(double), double a, double b, int n) {
    double h = (b - a) / n;
    double sum = (f(a) + f(b)) / 2.0;

    for (int i = 1; i < n; i++) {
        double x = a + i * h;
        sum += f(x);
    }

    return sum * h;
}

// Midpoint rule
double midpoint_integration(double (*f)(double), double a, double b, int n) {
    double h = (b - a) / n;
    double sum = 0.0;

    for (int i = 0; i < n; i++) {
        double x = a + (i + 0.5) * h;
        sum += f(x);
    }

    return sum * h;
}

// Simpson's rule
double simpsons_integration(double (*f)(double), double a, double b, int n) {
    if (n % 2 != 0) n++; // Ensure n is even for Simpson's rule
    double h = (b - a) / n;
    double sum = f(a) + f(b);

    for (int i = 1; i < n; i++) {
        double x = a + i * h;
        sum += (i % 2 == 0 ? 2 : 4) * f(x);
    }

    return sum * h / 3.0;
}

// Definite integral of a polynomial
double polynomial_integral(double coefficients[], int degree, double a, double b) {
    double integral_coefficients[degree + 2];
    for (int i = 0; i <= degree; i++) {
        integral_coefficients[i] = coefficients[i] / (degree - i + 1);
    }
    integral_coefficients[degree + 1] = 0; // Constant of integration

    // Evaluate the polynomial integral at b and a
    double result_b = evaluate_polynomial(integral_coefficients, degree + 1, b);
    double result_a = evaluate_polynomial(integral_coefficients, degree + 1, a);
    return result_b - result_a;
}

// Symbolic derivative of a polynomial
void polynomial_derivative(double coefficients[], int degree, double derivative_coefficients[]) {
    for (int i = 0; i < degree; i++) {
        derivative_coefficients[i] = coefficients[i] * (degree - i);
    }
}

double nrt(double x, double n) {
    if (n == 0) {
        // If n is 0, the n-th root is undefined; handle as an error
        return NAN; // Returns Not-a-Number
    }
    if (x < 0 && ((int)n) % 2 == 0) {
        // Even roots of negative numbers are undefined in real numbers
        return NAN;
    }
    return pow(x, 1.0 / n);
}

// Linear interpolation (lerp)
double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

// Struct for a 3D point
typedef struct {
    double x, y, z;
} Point3D;

// Helper function to calculate the cross product of two vectors
Point3D cross_product(Point3D u, Point3D v) {
    Point3D result;
    result.x = u.y * v.z - u.z * v.y;
    result.y = u.z * v.x - u.x * v.z;
    result.z = u.x * v.y - u.y * v.x;
    return result;
}

// Helper function to calculate the dot product of two vectors
double dot_product(Point3D u, Point3D v) {
    return u.x * v.x + u.y * v.y + u.z * v.z;
}

// Function to compute the volume of a tetrahedron
double tetrahedron_volume(Point3D A, Point3D B, Point3D C, Point3D D) {
    Point3D AB = {B.x - A.x, B.y - A.y, B.z - A.z};
    Point3D AC = {C.x - A.x, C.y - A.y, C.z - A.z};
    Point3D AD = {D.x - A.x, D.y - A.y, D.z - A.z};

    Point3D cross = cross_product(AC, AD);
    double volume = fabs(dot_product(AB, cross)) / 6.0;
    return volume;
}

// Function to compute the volume of a cube
double cube_volume(double side_length) {
    return side_length * side_length * side_length;
}

// Function to compute the surface area of a cube
double cube_surface_area(double side_length) {
    return 6 * side_length * side_length;
}

// Function to compute the volume of a sphere
double sphere_volume(double radius) {
    return (4.0 / 3.0) * PI * radius * radius * radius;
}

// Function to compute the surface area of a sphere
double sphere_surface_area(double radius) {
    return 4 * PI * radius * radius;
}

// Function to compute the volume of a cylinder
double cylinder_volume(double radius, double height) {
    return PI * radius * radius * height;
}

// Function to compute the surface area of a cylinder
double cylinder_surface_area(double radius, double height) {
    return 2 * PI * radius * (radius + height);
}

// Function to compute the volume of a cone
double cone_volume(double radius, double height) {
    return (1.0 / 3.0) * PI * radius * radius * height;
}

// Function to compute the surface area of a cone
double cone_surface_area(double radius, double height) {
    double slant_height = sqrt(radius * radius + height * height);
    return PI * radius * (radius + slant_height);
}

// Function to compute the volume of a rectangular prism
double rectangular_prism_volume(double length, double width, double height) {
    return length * width * height;
}

// Function to compute the surface area of a rectangular prism
double rectangular_prism_surface_area(double length, double width, double height) {
    return 2 * (length * width + width * height + height * length);
}

// Function to compute the volume of a pyramid
double pyramid_volume(double base_area, double height) {
    return (1.0 / 3.0) * base_area * height;
}

// Function to compute the volume of a torus
double torus_volume(double major_radius, double minor_radius) {
    return 2 * PI * PI * major_radius * minor_radius * minor_radius;
}

// Function to compute the surface area of a torus
double torus_surface_area(double major_radius, double minor_radius) {
    return 4 * PI * PI * major_radius * minor_radius;
}

// Basic mathematical utilities
double deg_to_rad(double degrees) {
    return degrees * (PI / 180.0);
}

double rad_to_deg(double radians) {
    return radians * (180.0 / PI);
}

// 2D transformations
void translate_2d(double* x, double* y, double dx, double dy) {
    *x += dx;
    *y += dy;
}

void scale_2d(double* x, double* y, double sx, double sy) {
    *x *= sx;
    *y *= sy;
}

void rotate_2d(double* x, double* y, double angle) {
    double radians = deg_to_rad(angle);
    double cos_a = cos(radians);
    double sin_a = sin(radians);
    double new_x = *x * cos_a - *y * sin_a;
    double new_y = *x * sin_a + *y * cos_a;
    *x = new_x;
    *y = new_y;
}

// 3D transformations
void translate_3d(double* x, double* y, double* z, double dx, double dy, double dz) {
    *x += dx;
    *y += dy;
    *z += dz;
}

void scale_3d(double* x, double* y, double* z, double sx, double sy, double sz) {
    *x *= sx;
    *y *= sy;
    *z *= sz;
}

void rotate_3d_x(double* y, double* z, double angle) {
    double radians = deg_to_rad(angle);
    double cos_a = cos(radians);
    double sin_a = sin(radians);
    double new_y = *y * cos_a - *z * sin_a;
    double new_z = *y * sin_a + *z * cos_a;
    *y = new_y;
    *z = new_z;
}

void rotate_3d_y(double* x, double* z, double angle) {
    double radians = deg_to_rad(angle);
    double cos_a = cos(radians);
    double sin_a = sin(radians);
    double new_x = *x * cos_a + *z * sin_a;
    double new_z = -(*x) * sin_a + *z * cos_a;
    *x = new_x;
    *z = new_z;
}

void rotate_3d_z(double* x, double* y, double angle) {
    double radians = deg_to_rad(angle);
    double cos_a = cos(radians);
    double sin_a = sin(radians);
    double new_x = *x * cos_a - *y * sin_a;
    double new_y = *x * sin_a + *y * cos_a;
    *x = new_x;
    *y = new_y;
}

// Distance calculations
double distance_2d(double x1, double y1, double x2, double y2) {
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

double distance_3d(double x1, double y1, double z1, double x2, double y2, double z2) {
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2) + pow(z2 - z1, 2));
}

// Graphing utilities
double linear(double x, double m, double c) {
    return m * x + c;
}

double quadratic(double x, double a, double b, double c) {
    return a * x * x + b * x + c;
}

double sine_wave(double x, double amplitude, double frequency, double phase) {
    return amplitude * sin(2 * PI * frequency * x + phase);
}

double cosine_wave(double x, double amplitude, double frequency, double phase) {
    return amplitude * cos(2 * PI * frequency * x + phase);
}

double sphere(double x, double y, double radius) {
    return sqrt(radius * radius - x * x - y * y);
}

double plane(double x, double y, double z0) {
    return z0;
}

// Polar to Cartesian conversion
void polar_to_cartesian_2d(double r, double theta, double* x, double* y) {
    double radians = deg_to_rad(theta);
    *x = r * cos(radians);
    *y = r * sin(radians);
}

// Spherical to Cartesian conversion
void spherical_to_cartesian(double r, double theta, double phi, double* x, double* y, double* z) {
    double theta_rad = deg_to_rad(theta);
    double phi_rad = deg_to_rad(phi);
    *x = r * sin(phi_rad) * cos(theta_rad);
    *y = r * sin(phi_rad) * sin(theta_rad);
    *z = r * cos(phi_rad);
}

// Bezier curve calculation (2D)
void bezier_curve_2d(double t, double x0, double y0, double x1, double y1, double x2, double y2, double* x, double* y) {
    double u = 1 - t;
    *x = u * u * x0 + 2 * u * t * x1 + t * t * x2;
    *y = u * u * y0 + 2 * u * t * y1 + t * t * y2;
}

// Bezier curve calculation (3D)
void bezier_curve_3d(double t, double x0, double y0, double z0, double x1, double y1, double z1, double x2, double y2, double z2, double* x, double* y, double* z) {
    double u = 1 - t;
    *x = u * u * x0 + 2 * u * t * x1 + t * t * x2;
    *y = u * u * y0 + 2 * u * t * y1 + t * t * y2;
    *z = u * u * z0 + 2 * u * t * z1 + t * t * z2;
}



// 4D to 3D projection function (ignores the w component)
void project_4d_to_3d(double x, double y, double z, double w, double* x_out, double* y_out, double* z_out) {
    // Simple projection: Ignore the 'w' component and keep (x, y, z)
    *x_out = x / (1 + w);  // Perspective divide for 'x'
    *y_out = y / (1 + w);  // Perspective divide for 'y'
    *z_out = z / (1 + w);  // Perspective divide for 'z'
}

// 4D translation (move a 4D point by a vector)
void translate_4d(double* x, double* y, double* z, double* w, double dx, double dy, double dz, double dw) {
    *x += dx;
    *y += dy;
    *z += dz;
    *w += dw;
}

// 4D scaling function
void scale_4d(double* x, double* y, double* z, double* w, double sx, double sy, double sz, double sw) {
    *x *= sx;
    *y *= sy;
    *z *= sz;
    *w *= sw;
}

// 4D rotation matrix around the X axis
void rotate_4d_x(double* y, double* z, double* w, double angle) {
    double radians = deg_to_rad(angle);
    double cos_a = cos(radians);
    double sin_a = sin(radians);
    double new_y = *y * cos_a - *z * sin_a;
    double new_z = *y * sin_a + *z * cos_a;
    double new_w = *w * cos_a;  // Keep 'w' component unaffected in this rotation.
    *y = new_y;
    *z = new_z;
    *w = new_w;
}

// 4D rotation matrix around the Y axis
void rotate_4d_y(double* x, double* z, double* w, double angle) {
    double radians = deg_to_rad(angle);
    double cos_a = cos(radians);
    double sin_a = sin(radians);
    double new_x = *x * cos_a + *z * sin_a;
    double new_z = -(*x) * sin_a + *z * cos_a;
    double new_w = *w * cos_a;  // Keep 'w' component unaffected in this rotation.
    *x = new_x;
    *z = new_z;
    *w = new_w;
}

// 4D rotation matrix around the Z axis
void rotate_4d_z(double* x, double* y, double* w, double angle) {
    double radians = deg_to_rad(angle);
    double cos_a = cos(radians);
    double sin_a = sin(radians);
    double new_x = *x * cos_a - *y * sin_a;
    double new_y = *x * sin_a + *y * cos_a;
    double new_w = *w * cos_a;  // Keep 'w' component unaffected in this rotation.
    *x = new_x;
    *y = new_y;
    *w = new_w;
}

// 4D rotation matrix around the W axis (a different dimension of rotation)
void rotate_4d_w(double* x, double* y, double* z, double* w, double angle) {
    double radians = deg_to_rad(angle);
    double cos_a = cos(radians);
    double sin_a = sin(radians);
    double new_x = *x * cos_a - *y * sin_a;
    double new_y = *x * sin_a + *y * cos_a;
    double new_z = *z * cos_a - *w * sin_a;
    double new_w = *z * sin_a + *w * cos_a;
    *x = new_x;
    *y = new_y;
    *z = new_z;
    *w = new_w;
}

// Function to calculate the distance between two 4D points
double distance_4d(double x1, double y1, double z1, double w1, double x2, double y2, double z2, double w2) {
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2) + pow(z2 - z1, 2) + pow(w2 - w1, 2));
}

// 4D to 3D projection with a specific matrix (homogeneous coordinates)
void project_4d_to_3d_matrix(double x, double y, double z, double w, double* x_out, double* y_out, double* z_out) {
    // For a more complex projection (can be extended with perspective projection)
    double factor = 1.0 / (1.0 + w);  // Simple perspective projection
    *x_out = x * factor;
    *y_out = y * factor;
    *z_out = z * factor;
}

// Example function to project a 4D hypercube into 3D
void project_hypercube_4d(double* vertices, int num_vertices, double* projected_vertices) {
    for (int i = 0; i < num_vertices; i++) {
        // Each vertex has 4 components (x, y, z, w), thus process them in a loop
        double x = vertices[i * 4];
        double y = vertices[i * 4 + 1];
        double z = vertices[i * 4 + 2];
        double w = vertices[i * 4 + 3];
        
        // Project the 4D point onto 3D space
        project_4d_to_3d(x, y, z, w, &projected_vertices[i * 3], &projected_vertices[i * 3 + 1], &projected_vertices[i * 3 + 2]);
    }
}
