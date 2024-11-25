#include "flopmath.h"

// Factorial function for Taylor series
static double factorial(int n) {
    double result = 1.0;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

// Power function
static double power(double base, int exp) {
    double result = 1.0;
    for (int i = 0; i < exp; i++) {
        result *= base;
    }
    return result;
}

// Exponentiation function (e^x)
double exp(double x) {
    double result = 1.0;
    double term = 1.0;
    for (int n = 1; n < 20; n++) {
        term *= x / n;
        result += term;
    }
    return result;
}

// Natural logarithm (ln(x)) using a Taylor series approximation around 1
double ln(double x) {
    if (x <= 0) {
        return NAN; // Logarithm undefined for x <= 0
    }

    double result = 0.0;
    double term = (x - 1) / (x + 1);
    double termSquared = term * term;
    double numerator = term;
    for (int n = 1; n < 20; n++) {
        result += numerator / (2 * n - 1);
        numerator *= termSquared;
    }
    return 2 * result;
}

// Sine function using Taylor series approximation
double sin(double x) {
    // Normalize x to range [-PI, PI] for better accuracy
    while (x > PI) x -= 2 * PI;
    while (x < -PI) x += 2 * PI;

    double result = 0.0;
    for (int n = 0; n < 10; n++) {
        double term = power(-1, n) * power(x, 2 * n + 1) / factorial(2 * n + 1);
        result += term;
    }
    return result;
}

// Cosine function using Taylor series approximation
double cos(double x) {
    // Normalize x to range [-PI, PI] for better accuracy
    while (x > PI) x -= 2 * PI;
    while (x < -PI) x += 2 * PI;

    double result = 0.0;
    for (int n = 0; n < 10; n++) {
        double term = power(-1, n) * power(x, 2 * n) / factorial(2 * n);
        result += term;
    }
    return result;
}

// Tangent function using sine and cosine
double tan(double x) {
    return sin(x) / cos(x);
}

// Absolute value for floating-point numbers
double fabs(double x) {
    return (x < 0) ? -x : x;
}

// Absolute value for integers
int abs(int x) {
    return (x < 0) ? -x : x;
}

// Square root function using Newton's method (approximation)
double sqrt(double x) {
    if (x < 0) return NAN; // Square root undefined for negative numbers
    double guess = x / 2.0;
    double epsilon = 1e-7;
    while (fabs(guess * guess - x) > epsilon) {
        guess = (guess + x / guess) / 2.0;
    }
    return guess;
}

// Logarithm base 10 function using natural logarithm
double log10(double x) {
    if (x <= 0) return NAN; // Logarithm undefined for x <= 0
    return ln(x) / ln(10);
}

// Power function (x^y)
double pow(double base, double exp) {
    return exp * ln(base);  // Using the relationship a^b = e^(b*ln(a))
}

// Hyperbolic sine (sinh) function using Taylor series
double sinh(double x) {
    double result = 0.0;
    for (int n = 0; n < 10; n++) {
        double term = power(x, 2 * n + 1) / factorial(2 * n + 1);
        result += term;
    }
    return result;
}

// Hyperbolic cosine (cosh) function using Taylor series
double cosh(double x) {
    double result = 1.0;
    for (int n = 1; n < 10; n++) {
        double term = power(x, 2 * n) / factorial(2 * n);
        result += term;
    }
    return result;
}

// Hyperbolic tangent (tanh) function using sinh and cosh
double tanh(double x) {
    return sinh(x) / cosh(x);
}

// Arcsine function (asin) using a numerical method (Newton's method approximation)
double asin(double x) {
    if (x < -1 || x > 1) return NAN; // arcsine is only valid for [-1, 1]
    double guess = x;
    double epsilon = 1e-7;
    while (fabs(sin(guess) - x) > epsilon) {
        guess -= (sin(guess) - x) / cos(guess); // Newton's method
    }
    return guess;
}

// Arccosine function (acos) using numerical method
double acos(double x) {
    return PI / 2 - asin(x);
}

// Arctangent function (atan) using numerical method
double atan(double x) {
    double result = 0.0;
    double term = x;
    for (int n = 0; n < 10; n++) {
        result += term / (2 * n + 1);
        term *= -x * x;
    }
    return result;
}

// Arctangent of y/x (atan2) using atan
double atan2(double y, double x) {
    if (x > 0) return atan(y / x);
    if (x < 0) return atan(y / x) + PI;
    if (y > 0) return PI / 2;
    if (y < 0) return -PI / 2;
    return NAN; // Undefined for y = 0, x = 0
}

// Rounding function (round)
double round(double x) {
    return (x > 0) ? floor(x + 0.5) : ceil(x - 0.5);
}

// Floor function (largest integer less than or equal to x)
double floor(double x) {
    return (x == (int)x) ? x : (x > 0) ? (int)x : (int)x - 1;
}

// Ceil function (smallest integer greater than or equal to x)
double ceil(double x) {
    return (x == (int)x) ? x : (x > 0) ? (int)x + 1 : (int)x;
}

// Min function
double min(double a, double b) {
    return (a < b) ? a : b;
}

// Max function
double max(double a, double b) {
    return (a > b) ? a : b;
}
