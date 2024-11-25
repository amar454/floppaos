#ifndef FLOPMATH_H
#define FLOPMATH_H

// Constant for PI
#define PI 3.14159265358979323846

// Define NAN using a specific bit pattern (IEEE 754 standard for NaN)
#define NAN (*(volatile double*)(&((unsigned long long)0x7ff8000000000000)))

// Declaration for the mathematical functions

// Factorial function for Taylor series
double factorial(int n);

// Power function (base^exp)
double power(double base, int exp);

// Exponentiation function (e^x)
double exp(double x);

// Natural logarithm (ln(x)) using a Taylor series approximation around 1
double ln(double x);

// Sine function using Taylor series approximation
double sin(double x);

// Cosine function using Taylor series approximation
double cos(double x);

// Tangent function using sine and cosine
double tan(double x);

// Absolute value for floating-point numbers
double fabs(double x);

// Absolute value for integers
int abs(int x);

// Square root function using Newton's method (approximation)
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

#endif // FLOPMATH_H
