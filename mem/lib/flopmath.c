
#define PI 3.14159265

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
// Absolute value for floating-point numbers
double fabs(double x) {
    return (x < 0) ? -x : x;
}

// Absolute value for integers
int abs(int x) {
    return (x < 0) ? -x : x;
}