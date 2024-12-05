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
