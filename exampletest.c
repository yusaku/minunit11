#include "minunit.h"

TEST(success)
{
    printf("success\n");
    ASSERT_EQ(1, 1);
}

TEST(failure)
{
    printf("failure\n");
    ASSERT_NE(1, 1);
}

START() {
    RUN(success);
    RUN(failure);
}
