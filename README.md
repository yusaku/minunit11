# MinUnit11
A minimal unit testing framework for C11.

This implements a minimal unit testing framework whose basic idea comes from "[minunit.h](http://www.jera.com/techinfo/jtns/jtn002.html)" and its successor explained in a modern C textbook "Learn C the hard way" by Zed A. Shaw.

# Features

This library has the following features:
- Colored result: This outputs test results with colors in a similar way to [Green](https://github.com/CleanCut/green), a nice test runner for Python.
- Captured output: This captures the output of tested code and shows it separately from test results.
- Assertions with auto inspection: This automatically insepcts values in failed assertions for debugging.

## Examples

A typical test code looks as follows:
```c
// saved as exampletest.c

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
```

Compile it with `cc -std=c11 exampletest.c -o exampletest` and run `./exampletest`.

