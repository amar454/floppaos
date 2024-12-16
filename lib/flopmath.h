#ifndef FLOPMATH_H
#define FLOPMATH_H

#include <math.h>

// Constants
#define PI 3.14159265358979323846
#define NAN 1.0/1.0

// Factorial function for Taylor series
double factorial(int n);

// Power function (base^exp)
double power(double base, int exp);

// Exponentiation function (e^x)
double exp(double x);

// Natural logarithm (ln(x))
double ln(double x);

// Adjust angle to range [0, 2*PI]
double normalize_angle(double x);

// Sine function using Taylor series
double sin(double x);

// Cosine function using Taylor series
double cos(double x);

// Tangent function using sine and cosine
double tan(double x);

// Absolute value for floating-point numbers
double fabs(double x);

// Absolute value for integers
int abs(int x);

// Square root function using Newton's method
double sqrt(double x);

// Logarithm base 10 function using natural logarithm
double log10(double x);

// Power function (x^y) using the relationship a^b = e^(b*ln(a))
double pow(double base, double exp);

// Hyperbolic sine (sinh) function using Taylor series
double sinh(double x);

// Hyperbolic cosine (cosh) function using Taylor series
double cosh(double x);

// Hyperbolic tangent (tanh) function using sinh and cosh
double tanh(double x);

// Arcsine function (asin) using Newton's method approximation
double asin(double x);

// Arccosine function (acos) using numerical method
double acos(double x);

// Arctangent function (atan) using numerical method
double atan(double x);

// Arctangent of y/x (atan2) using atan
double atan2(double y, double x);

// Rounding function (round)
double round(double x);

// Floor function (largest integer less than or equal to x)
double floor(double x);

// Ceil function (smallest integer greater than or equal to x)
double ceil(double x);

// Min function
double min(double a, double b);

// Max function
double max(double a, double b);

// Absolute value function (for integer types)
int abs_int(int x);

// Logarithm base 2 (log2)
double log2(double x);

// Cube root function (cbrt)
double cbrt(double x);

// Greatest common divisor (gcd) of two integers
int gcd(int a, int b);

// Least common multiple (lcm) of two integers
int lcm(int a, int b);

// Hyperbolic secant (sech)
double sech(double x);

// Hyperbolic cosecant (csch)
double csch(double x);

// Hyperbolic cotangent (coth)
double coth(double x);

// Computes the sine of an angle in degrees (uses sin function for radians)
double sind(double x);

// Computes the cosine of an angle in degrees (uses cos function for radians)
double cosd(double x);

// Computes the tangent of an angle in degrees (uses tan function for radians)
double tand(double x);

// Computes the cotangent of an angle (1/tan(x))
double cot(double x);

// Computes the secant of an angle (1/cos(x))
double sec(double x);

// Computes the cosecant of an angle (1/sin(x))
double csc(double x);

// Computes the exponential of a base raised to the power of an exponent (a^b)
double exp_base(double a, double b);

// Finds the nth Fibonacci number using an iterative approach
long long fib(int n);

// Computes the mean of an array of numbers
double mean(double arr[], int size);

// Computes the variance of an array of numbers
double variance(double arr[], int size);

// Computes the standard deviation of an array of numbers
double stddev(double arr[], int size);

// Computes the factorial of a number (int) recursively
long long factorial_recursive(int n);

// Vector3 struct definition for 3D vectors
typedef struct {
    double x, y, z;
} Vector3;

// Vector operations
Vector3 vector_add(Vector3 a, Vector3 b);
Vector3 vector_subtract(Vector3 a, Vector3 b);
Vector3 vector_scalar_multiply(Vector3 v, double scalar);
double vector_dot(Vector3 a, Vector3 b);
Vector3 vector_cross(Vector3 a, Vector3 b);
double vector_magnitude(Vector3 v);
Vector3 vector_normalize(Vector3 v);
double vector_angle_between(Vector3 a, Vector3 b);
Vector3 vector_project(Vector3 a, Vector3 b);

// Matrix operations
typedef struct {
    double m[4][4];
} Matrix4x4;

// Identity matrix
Matrix4x4 matrix_identity();

// Matrix multiplication
Matrix4x4 matrix_multiply(Matrix4x4 a, Matrix4x4 b);

// Matrix translation
Matrix4x4 matrix_translation(double tx, double ty, double tz);

// Matrix scaling
Matrix4x4 matrix_scaling(double sx, double sy, double sz);

// Matrix rotation around X-axis
Matrix4x4 matrix_rotate_x(double angle);

// Matrix rotation around Y-axis
Matrix4x4 matrix_rotate_y(double angle);

// Matrix rotation around Z-axis
Matrix4x4 matrix_rotate_z(double angle);

// Perspective projection matrix
Matrix4x4 matrix_perspective(double fov, double aspect, double near, double far);

// LookAt view matrix
Matrix4x4 matrix_look_at(Vector3 eye, Vector3 target, Vector3 up);

// Linear interpolation (lerp)
double lerp(double a, double b, double t);

// Smoothstep (easing function)
double smoothstep(double edge0, double edge1, double x);

// Random number generation (uniform)
double rand_range(double min, double max);

#endif // FLOPMATH_H
