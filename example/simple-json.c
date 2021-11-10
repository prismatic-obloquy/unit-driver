// Notice this #define at the top
#define UD_OUTPUT_FORMAT 'j'
#include "unit-driver.h"

#include "lib.h"

void add1_1_eq_2(const void* _) {
    (void) _;
    int result = add1(1);
    UDA_EQ(result, 2);
}

void add1_1_ne_3(const void* _) {
    (void) _;
    int result = add1(1);
    // intentional bug in the test to demonstrate assert failures
    UDA_EQ(result, 3);
    UDA_NE(result, 2);
}

UD_REGISTER_TESTS {
    // the output format of this one is JSON, thanks to the #define at the top
    UD_SUITE(add1, NULL, NULL);
    UD_TEST(add1, add1_1_eq_2);
    UD_TEST(add1, add1_1_ne_3);
}
