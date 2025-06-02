#ifndef FLOPMATH_H
#define FLOPMATH_H


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

int solve_quadratic(double a, double b, double c, double *root1, double *root2);

double determinant_2x2(double a, double b, double c, double d);

double determinant_3x3(double m[3][3]);
// Random number generation (uniform)
double rand_range(double min, double max);

double compute_lighting(Vector3 normal, Vector3 light_dir);

int solve_linear_system(int n, double matrix[][n + 1], double results[]);

double evaluate_polynomial(double coefficients[], int degree, double x);

double numerical_differentiation(double (*f)(double), double x, double h);

double trapezoidal_integration(double (*f)(double), double a, double b, int n);

double midpoint_integration(double (*f)(double), double a, double b, int n);

double simpsons_integration(double (*f)(double), double a, double b, int n);

double polynomial_integral(double coefficients[], int degree, double a, double b);

void polynomial_derivative(double coefficients[], int degree, double derivative_coefficients[]);

double nrt(double x, double n);

double cube_volume(double side_length);

double cube_surface_area(double side_length);

double sphere_volume(double radius);

double sphere_surface_area(double radius);

double cylinder_volume(double radius, double height);

double cylinder_surface_area(double radius, double height);

double cone_volume(double radius, double height);

double cone_surface_area(double radius, double height);

double rectangular_prism_volume(double length, double width, double height);

double rectangular_prism_surface_area(double length, double width, double height);

double pyramid_volume(double base_area, double height);

double torus_volume(double major_radius, double minor_radius);

double torus_surface_area(double major_radius, double minor_radius);

double deg_to_rad(double degrees);

double rad_to_deg(double radians);

void translate_2d(double* x, double* y, double dx, double dy);

void scale_2d(double* x, double* y, double sx, double sy);

void rotate_2d(double* x, double* y, double angle);

void translate_3d(double* x, double* y, double* z, double dx, double dy, double dz);

void scale_3d(double* x, double* y, double* z, double sx, double sy, double sz);

void rotate_3d_x(double* y, double* z, double angle);

void rotate_3d_y(double* x, double* z, double angle);

void rotate_3d_z(double* x, double* y, double angle);

double distance_2d(double x1, double y1, double x2, double y2);

double distance_3d(double x1, double y1, double z1, double x2, double y2, double z2);

double linear(double x, double m, double c);

double quadratic(double x, double a, double b, double c);

double sine_wave(double x, double amplitude, double frequency, double phase);

double cosine_wave(double x, double amplitude, double frequency, double phase);

double sphere(double x, double y, double radius);

double plane(double x, double y, double z0);

void polar_to_cartesian_2d(double r, double theta, double* x, double* y);

void spherical_to_cartesian(double r, double theta, double phi, double* x, double* y, double* z);

void bezier_curve_2d(double t, double x0, double y0, double x1, double y1, double x2, double y2, double* x, double* y);

void bezier_curve_3d(double t, double x0, double y0, double z0, double x1, double y1, double z1, double x2, double y2, double z2, double* x, double* y, double* z);

void project_4d_to_3d(double x, double y, double z, double w, double* x_out, double* y_out, double* z_out);

void translate_4d(double* x, double* y, double* z, double* w, double dx, double dy, double dz, double dw);

void scale_4d(double* x, double* y, double* z, double* w, double sx, double sy, double sz, double sw);

void rotate_4d_x(double* y, double* z, double* w, double angle);

void rotate_4d_y(double* x, double* z, double* w, double angle);

void rotate_4d_z(double* x, double* y, double* w, double angle);

void rotate_4d_w(double* x, double* y, double* z, double* w, double angle);

double distance_4d(double x1, double y1, double z1, double w1, double x2, double y2, double z2, double w2);

void project_4d_to_3d_matrix(double x, double y, double z, double w, double* x_out, double* y_out, double* z_out);

void project_hypercube_4d(double* vertices, int num_vertices, double* projected_vertices);
#endif // FLOPMATH_H
