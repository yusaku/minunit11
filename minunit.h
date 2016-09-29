/* Copyright (c) 2016 Yusaku Kaneta
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __MINUNIT_H__
#define __MINUNIT_H__

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// APIs
#define START()         static void MinUnit_execute(struct MinUnit *mu_)
#define TEST(name)      static void name(struct MinUnit *mu)
#define RUN(name)       if (MinUnit_run(mu_, name, #name) == EXIT_SUCCESS) { mu_->success++; } else { mu_->failure++; if (muconf->x) return; }
#define TESTLOG(...)    fprintf(muerr, __VA_ARGS__)

// Assertions
#define ASSERT_EQ(x1, x2) MinUnit_assert(mu, x1, x2, ==, !=)
#define ASSERT_NE(x1, x2) MinUnit_assert(mu, x1, x2, !=, ==)
#define ASSERT_LT(x1, x2) MinUnit_assert(mu, x1, x2, < , >=)
#define ASSERT_LE(x1, x2) MinUnit_assert(mu, x1, x2, <=, > )
#define ASSERT_GT(x1, x2) MinUnit_assert(mu, x1, x2, > , >=)
#define ASSERT_GE(x1, x2) MinUnit_assert(mu, x1, x2, >=, > )

// Colors
#define BOLD(msg)   "\033[1m"  msg "\033[0m"
#define PASS(msg)   "\033[32m" msg "\033[0m"
#define FAIL(msg)   "\033[31m" msg "\033[0m"
#define INFO(msg)   "\033[34m" msg "\033[0m"

// Internals
#define MinUnit_typespec(code)\
_Generic((code),\
char *            : "%s"  ,\
signed char       : "%hhd",\
signed short      : "%hd" ,\
signed int        : "%d"  ,\
signed long       : "%ld" ,\
signed long long  : "%lld",\
unsigned char     : "%u"  ,\
unsigned short    : "%hu" ,\
unsigned int      : "%u"  ,\
unsigned long     : "%lu" ,\
unsigned long long: "%llu",\
float             : "%f"  ,\
double            : "%lf" ,\
long double       : "%Lf" ,\
default           : "%p")

#define MinUnit_assert(mu, x1, x2, op, notop)\
if (!((x1) op (x2))) {\
    fprintf((mu)->faillog, "  Assertion failed: ");\
    fprintf((mu)->faillog, MinUnit_typespec(x1), x1);\
    fprintf((mu)->faillog, " " #notop " ");\
    fprintf((mu)->faillog, MinUnit_typespec(x2), x2);\
    fprintf((mu)->faillog, " (%s:%d)\n", __FILE__, __LINE__);\
    (mu)->failure++; return; }

struct MinUnit { double elapsed; int success; int failure; FILE *testlog; FILE *faillog; };
struct MinUnitConf { bool q; bool s; bool v; bool x; };
typedef void (*MinUnit_func_t)(struct MinUnit *mu);
static void MinUnit_execute(struct MinUnit *mu);

static struct MinUnitConf *muconf = &(struct MinUnitConf) { .q=false, .s=false, .v=false, .x=false };
static FILE *muout = NULL;
static FILE *muerr = NULL;

static void MinUnit_cleanup(struct MinUnit *mu)
{
    if (mu->testlog) fclose(mu->testlog);
    if (mu->faillog) fclose(mu->faillog);
}

static double MinUnit_gettime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

static int MinUnit_call(struct MinUnit *mu, MinUnit_func_t func)
{
    double start = MinUnit_gettime();
    func(mu);
    mu->elapsed = MinUnit_gettime() - start;
    return (mu->failure == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void MinUnit_copy(FILE *src, FILE *dst)
{
    rewind(src);
    for (int c = getc(src); c != EOF; c = getc(src)) fputc(c, dst);
}

static bool MinUnit_empty(FILE *file)
{
    fseek(file, 0, SEEK_END);
    return ftell(file) == 0;
}

static FILE *MinUnit_tmpfile()
{
    FILE *file = tmpfile();
    if (file == NULL) {
        TESTLOG("ERROR: tmpfile failed\n");
        exit(EXIT_FAILURE);
    }
    return file;
}

static int MinUnit_run(struct MinUnit *mu, MinUnit_func_t func, const char *name)
{
    struct MinUnit *running = &(struct MinUnit) {
        .elapsed=0, .success=0, .failure=0, .testlog=MinUnit_tmpfile(), .faillog=MinUnit_tmpfile() };

    if (!muconf->s) {
        stdout = running->testlog;
        stderr = running->testlog;
    }

    int rc = MinUnit_call(running, func);

    if (running->failure == 0) {
        TESTLOG(PASS("."));
        if (muconf->v) TESTLOG(PASS("  %s\n"), name);
    } else {
        TESTLOG(FAIL("F"));
        if (muconf->v) TESTLOG(FAIL("  %s\n"), name);
    }

    if (!MinUnit_empty(running->faillog)) {
        fprintf(mu->testlog, FAIL("\nFAILURE")" in " BOLD("%s\n"), name);
        MinUnit_copy(running->faillog, mu->testlog);
    }

    if (!muconf->q) {
        if (!MinUnit_empty(running->testlog)) {
            fprintf(mu->testlog, INFO("\nCAPTURED STDOUT/STDERR")" for "BOLD("%s\n"), name);
            MinUnit_copy(running->testlog, mu->testlog);
        }
    }

    MinUnit_cleanup(running);
    return rc;
}

static void MinUnit_usage(const char *cmd)
{
    TESTLOG("usage: %s [-qsvx]\n"
                    "Options:\n"
                    "  -q  Quiet stdout.\n"
                    "  -s  Disable to capture stdout.\n"
                    "  -v  Enalbe verbose mode.\n"
                    "  -x  Exit on first failure.\n", cmd);
}

static void MinUnit_optparse(int argc, char **argv)
{
    for (size_t i = 1; i < (size_t)argc; i++) {
        if (argv[i][0] != '-') {
            MinUnit_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
                
        for (size_t j = 1; j < strlen(argv[i]); j++) {
            switch (argv[i][j]) {
                case 's': muconf->s = true; break;
                case 'v': muconf->v = true; break; 
                case 'x': muconf->x = true; break;
                case 'q': muconf->q = true; break;
                case 'h':
                    MinUnit_usage(argv[0]);
                    exit(EXIT_SUCCESS);
                default :
                    TESTLOG("%s: illegal option -- %c\n", argv[0], argv[i][j]);
                    MinUnit_usage(argv[0]);
                    exit(EXIT_FAILURE);
            }
        }
    }
}

int main(int argc, char **argv)
{
    muout = stdout;
    muerr = stderr;

    MinUnit_optparse(argc, argv);

    struct MinUnit *mu = &(struct MinUnit) { .success=0, .failure=0, .testlog=MinUnit_tmpfile(), .faillog=NULL };
    int rc = MinUnit_call(mu, MinUnit_execute);

    if (!muconf->v) fputc('\n', muerr);
    MinUnit_copy(mu->testlog, muerr);
    TESTLOG("\nRAN "BOLD("%d")" TESTS IN "BOLD("%4.3lf")"s\n", mu->success + mu->failure, mu->elapsed);

    if ((mu->success + mu->failure) > 0) {
        TESTLOG("\n%s (SUCCESS: "PASS("%d")", FAILURE: "FAIL("%d")")\n", (mu->failure == 0) ? PASS("OK") : FAIL("FAILED"), mu->success, mu->failure);
    } else {
        TESTLOG(FAIL("\nNO TESTS FOUND\n"));
    }

    MinUnit_cleanup(mu);
    return rc;
}

#endif
