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
    UDA_NE(result, 2);
}

UD_REGISTER_TESTS {
    UD_SUITE(add1, NULL, NULL);
    UD_TEST(add1, add1_1_eq_2);
    UD_TEST(add1, add1_1_ne_3);
}
