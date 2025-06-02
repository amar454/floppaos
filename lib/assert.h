#include <stdint.h>
#include "../drivers/vga/vgahandler.h"
#include "../lib/logging.h"

#include "../kernel.h"


#define ASSERT(expr) if (!(expr)) { \
    log("Assertion failed: " #expr "\n", RED);  \
    log("File: " __FILE__ "\nLine: " __LINE__ "\n", RED); \
    halt(); \
}

#define ASSERT_MSG(expr, msg) \
    do { \
        if (!(expr)) { \
            log("Assertion failed: " #expr "\nMessage: " msg "\n", RED);\
            log("File: " __FILE__ "\nLine: " __LINE__ "\n", RED); \
            halt(); \
        } \
    } while (0)

