#ifndef FLOPMATH_H
#define FLOPMATH_H

// Constant for PI
#define PI 3.14159265358979323846

// Define NAN using a specific bit pattern (IEEE 754 standard for NaN)
#define NAN (0.0 / 0.0)

// Declaration for the mathematical functions

double factorial(int n);
double power(double base, int exp);
double exp(double x);
double ln(double x);
double sin(double x);
double cos(double x);
double tan(double x);
double fabs(double x);
int abs(int x);
double sqrt(double x);
double log10(double x);
double pow(double base, double exp);
double sinh(double x);
double cosh(double x);
double tanh(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double atan2(double y, double x);
double round(double x);
double floor(double x);
double ceil(double x);
double min(double a, double b);
double max(double a, double b);

#endif // FLOPMATH_H
