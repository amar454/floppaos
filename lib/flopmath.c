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

// Returns the absolute value of an integer (already implemented but defined here for clarity)
int abs_int(int x) {
    return (x < 0) ? -x : x;
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

// Vector3 struct definition for 3D vectors
typedef struct {
    double x, y, z;
} Vector3;

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

// Matrix operations

typedef struct {
    double m[4][4];
} Matrix4x4;

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

double compute_lighting(Vector3D normal, Vector3D light_dir) {
    normal = vector_normalize(normal);
    light_dir = vector_normalize(light_dir);
    double intensity = vector_dot(normal, light_dir);
    return fmax(0.0, intensity); // Clamp to [0, 1]
}
// Solve a linear system using Gauss-Jordan Elimination (NxN matrix)
int solve_linear_system(double matrix[][N], double results[], int n) {
    for (int i = 0; i < n; i++) {
        // Make the diagonal element 1
        double diag = matrix[i][i];
        if (fabs(diag) < 1e-9) return 0; // Singular matrix
        for (int j = 0; j <= n; j++) matrix[i][j] /= diag;

        // Eliminate other rows
        for (int k = 0; k < n; k++) {
            if (k != i) {
                double factor = matrix[k][i];
                for (int j = 0; j <= n; j++) {
                    matrix[k][j] -= factor * matrix[i][j];
                }
            }
        }
    }

    for (int i = 0; i < n; i++) results[i] = matrix[i][n];
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

// Numerical integration (trapezoidal rule)
double numerical_integration(double (*f)(double), double a, double b, int n) {
    double h = (b - a) / n;
    double sum = 0.5 * (f(a) + f(b));
    for (int i = 1; i < n; i++) {
        sum += f(a + i * h);
    }
    return sum * h;
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

// Compute lighting using Lambertian reflection
double compute_lighting(Vector3D normal, Vector3D light_dir) {
    normal = vector_normalize(normal);
    light_dir = vector_normalize(light_dir);
    double intensity = vector_dot(normal, light_dir);
    return max(0.0, intensity); // Clamp to [0, 1]
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


