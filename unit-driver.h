/***************************************************************************\
|                          NOTE TO DEVELOPERS                               |
+---------------------------------------------------------------------------+
| This is Unit Driver. It just orchestrates the tests. You shouldn't edit   |
| it unless you're changing *how* tests work. If you just want to add or    |
| remove tests, see all-tests.c. To change them, look for the relevant call |
| to UD_SUITE and UD_TEST, then edit the functions that those use. Don't    |
| edit this file unless you know what you're doing.                         |
+---------------------------------------------------------------------------+
| Source and docs at https://github.com/prismatic-obloquy/unit-driver       |
+---------------------------------------------------------------------------+
| Copyright 2021 Prismatic Obloquy                                          |
|                                                                           |
| Redistribution and use in source and binary forms, with or without        |
| modification, are permitted provided that the following conditions are    |
| met:                                                                      |
|                                                                           |
| 1. Redistributions of source code must retain the above copyright notice, |
| this list of conditions and the following disclaimer.                     |
|                                                                           |
| 2. Redistributions in binary form must reproduce the above copyright      |
| notice, this list of conditions and the following disclaimer in the       |
| documentation and/or other materials provided with the distribution.      |
|                                                                           |
| 3. Neither the name of the copyright holder nor the names of its          |
| contributors may be used to endorse or promote products derived from this |
| software without specific prior written permission.                       |
|                                                                           |
| THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS   |
| IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, |
| THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR    |
| PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE  COPYRIGHT HOLDER OR        |
| CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,     |
| EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       |
| PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR        |
| PROFITS; OR BUSINESS INTERRUPTION) HOWEVER  CAUSED AND ON ANY THEORY OF   |
| LIABILITY, WHETHER IN CONTRACT, STRICT  LIABILITY, OR TORT (INCLUDING     |
| NEGLIGENCE OR OTHERWISE) ARISING IN  ANY WAY OUT OF THE USE OF THIS       |
| SOFTWARE, EVEN IF ADVISED OF THE  POSSIBILITY OF SUCH DAMAGE.             |
\***************************************************************************/

#ifdef __UD_PREVIOUSLY__
#   error Cannot include UnitDriver twice.
#endif
#define __UD_PREVIOUSLY__

#ifndef UD_THREAD_COUNT
#   define UD_THREAD_COUNT 0
#endif

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#if UD_THREAD_COUNT > 0
#   include <threads.h>
#   include <stdatomic.h>
#endif

#define UDi_L_FIELDS(elem_t, name) \
    elem_t* name##_ary; int name##_cap; int name##_len

#define UDi_L_INIT(name) \
    .name##_ary = NULL, .name##_cap = 0, .name##_len = 0

#define UDi_L_ADD(cont, name, elem, success) do { \
    if (cont->name##_cap == cont->name##_len) { \
        int new_cap = cont->name##_cap ? cont->name##_cap * 2 : 4; \
        void* new = realloc(cont->name##_ary, \
                            new_cap * sizeof(cont->name##_ary[0])); \
        if (new == NULL) { \
            success = false; \
        } else { \
            success = true; \
            cont->name##_ary = new; \
            cont->name##_cap = new_cap; \
        } \
    } else { \
        success = true; \
    } \
    if (success) { \
        cont->name##_ary[cont->name##_len++] = elem; \
    } \
} while (0)

#define UDi_L_FOREACH(cont, name, elem_t, elem) \
    for ( \
        elem_t* elem = cont->name##_ary; \
        elem < cont->name##_ary + cont->name##_len; \
        elem++ \
    )

typedef void* (*UD_init_f)(void);
typedef void (*UD_clean_f)(void*);
typedef void (*UD_test_f)(const void*);

typedef struct {
    // note: not obvious, but fail_msg is the only allocated memory here
    const char* location;
    const char* text;
} UDi_assert;

typedef struct {
    const char* name;
    UD_test_f run;

    UDi_L_FIELDS(UDi_assert, asserts);
} UDi_test;

typedef struct {
    const char* name;
    UD_init_f init;
    UD_clean_f clean;

    UDi_L_FIELDS(UDi_test, tests);
} UDi_suite;

typedef struct {
    UDi_L_FIELDS(UDi_suite, suites);
} UDi_registry;

static UDi_registry UDi_reg = {
    UDi_L_INIT(suites),
};

UDi_suite* UDi_add_suite(const char* name, UD_init_f init, UD_clean_f clean) {
    UDi_suite new = {
        .name = name,
        .init = init,
        .clean = clean,
        UDi_L_INIT(tests),
    };
    bool succeeded;
    UDi_L_ADD((&UDi_reg), suites, new, succeeded);
    if (!succeeded) {
        return NULL;
    } else {
        return &UDi_reg.suites_ary[UDi_reg.suites_len-1];
    }
}

UDi_test* UDi_add_test(UDi_suite* s, const char* name, UD_test_f run) {
    UDi_test new = {
        .name = name,
        .run = run,
        UDi_L_INIT(asserts),
    };
    bool succeeded;
    UDi_L_ADD(s, tests, new, succeeded);
    if (!succeeded) {
        return NULL;
    } else {
        return &s->tests_ary[s->tests_len-1];
    }
}

UDi_assert* UDi_add_assert(
    UDi_test* t, const char* location,
    const char* text
) {
    UDi_assert new = {
        .location = location,
        .text = text,
    };
    bool succeeded;
    UDi_L_ADD(t, asserts, new, succeeded);
    if (!succeeded) {
        return NULL;
    } else {
        return &t->asserts_ary[t->asserts_len-1];
    }
}

void UDi_free_suites(void) {
    UDi_L_FOREACH((&UDi_reg), suites, UDi_suite, s) {
        UDi_L_FOREACH(s, tests, UDi_test, t) {
            // nothing to free in each assert, so just free the whole array
            free(t->asserts_ary);
        }
        free(s->tests_ary);
    }
    free(UDi_reg.suites_ary);
    // only resetting the outer layer because the rest are inaccessible once
    // it's nulled out
    UDi_reg = (UDi_registry) {
        UDi_L_INIT(suites),
    };
}

const char* UDi_asprintf(const char* fmt, ...) {
    va_list count_args;
    va_start(count_args, fmt);
    va_list fmt_args;
    va_copy(fmt_args, count_args);
    int size = vsnprintf(NULL, 0, fmt, count_args);
    if (size < 0) { return NULL; }
    char* res = malloc(size + 1);
    if (res == NULL) { return NULL; }
    vsnprintf(res, size, fmt, fmt_args);
    return res;
}

void UDi_setup_tests(bool*);

#if UD_THREAD_COUNT > 0
thread_local
#endif
UDi_test* UDi_current_test;
void UDi_run_test(UDi_test* test, const void* data) {
    UDi_current_test = test;
    test->run(data);
}

#if UD_THREAD_COUNT > 0
typedef struct {
    _Atomic bool running;
    struct {
        mtx_t lock;
        UDi_test* test;
        const void* data;
    } items[UD_THREAD_COUNT * 2];
} UDi_queue;

bool UDi_next_test(UDi_queue* queue, UDi_test** o_test, const void** o_data) {
    for (int i = 0; i < UD_THREAD_COUNT * 2; ++i) {
        if (thrd_busy == mtx_trylock(&queue->items[i].lock)) {
            continue;
        }
        if (queue->items[i].test == NULL) {
            mtx_unlock(&queue->items[i].lock);
            continue;
        }
        *o_test = queue->items[i].test;
        *o_data = queue->items[i].data;
        queue->items[i].test = NULL;
        queue->items[i].data = NULL;
        mtx_unlock(&queue->items[i].lock);
        return true;
    }
    return false;
}

int UDi_pull_tests(void* v_queue) {
    UDi_queue* queue = v_queue;
    UDi_test* test;
    const void* data;
    while (queue->running) {
        if (!UDi_next_test(queue, &test, &data)) {
            thrd_sleep(&(struct timespec) { .tv_nsec = 5000000 }, NULL);
            continue;
        }
        printf("Found test %s; running\n", test->name);
        UDi_run_test(test, data);
    }
    // once the queue is stopped, empty it before finishing up
    while (UDi_next_test(queue, &test, &data)) {
        printf("Found test %s; running\n", test->name);
        UDi_run_test(test, data);
    }
    return 0;
}
#endif

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;
    // this is slightly awkward, but means we don't need a `return` at the
    // end of the UDi_setup_tests definition, which is convenient.
    bool succeeded = true;
    UDi_setup_tests(&succeeded);
    if (!succeeded) {
        puts("Failed to set up tests");
        UDi_free_suites();
        return 1;
    }

#if UD_THREAD_COUNT > 0
    // Generate all the data first
    void* datae[UDi_reg.suites_len]; // plural of the plural :)
    for (int i = 0; i < UDi_reg.suites_len; ++i) {
        if (UDi_reg.suites_ary[i].init != NULL) {
            datae[i] = UDi_reg.suites_ary[i].init();
        } else {
            datae[i] = NULL;
        }
    }
    thrd_t threads[UD_THREAD_COUNT];
    UDi_queue queue = {
        .running = true,
    };
    for (int i = 0; i < UD_THREAD_COUNT * 2; ++i) {
        mtx_init(&queue.items[i].lock, mtx_plain);
        queue.items[i].test = NULL;
        queue.items[i].data = NULL;
    }
    for (int i = 0; i < UD_THREAD_COUNT; ++i) {
        thrd_create(&threads[i], UDi_pull_tests, &queue);
    }
    for (int i = 0; i < UDi_reg.suites_len; ++i) {
        UDi_L_FOREACH((&UDi_reg.suites_ary[i]), tests, UDi_test, t) {
            while (true) {
                for (int i = 0; i < UD_THREAD_COUNT * 2; ++i) {
                    if (thrd_busy == mtx_trylock(&queue.items[i].lock)) {
                        continue;
                    }
                    if (queue.items[i].test != NULL) {
                        mtx_unlock(&queue.items[i].lock);
                        continue;
                    }
                    queue.items[i].data = datae[i];
                    queue.items[i].test = t;
                    mtx_unlock(&queue.items[i].lock);
                    goto next_test;
                }
                const int SLEEP = 5000000 / UD_THREAD_COUNT;
                thrd_sleep(&(struct timespec) { .tv_nsec = SLEEP }, NULL);
            }
            next_test: (void)0;
        }
    }
    queue.running = false;
    for (int i = 0; i < UD_THREAD_COUNT; ++i) {
        thrd_join(threads[i], NULL);
    }
    for (int i = 0; i < UDi_reg.suites_len; ++i) {
        for (int i = 0; i < UDi_reg.suites_len; ++i) {
            if (UDi_reg.suites_ary[i].clean != NULL) {
                UDi_reg.suites_ary[i].clean(datae[i]);
                datae[i] = NULL;
            }
        }
    }
#else
    UDi_L_FOREACH((&UDi_reg), suites, UDi_suite, s) {
        void* data = s->init ? s->init() : NULL;
        UDi_L_FOREACH(s, tests, UDi_test, t) {
            UDi_run_test(t, data);
        }
        if (s->clean) { s->clean(data); };
    }
#endif

    // Generate the summary. We loop through once to learn:
    // - Number of suites, tests, and asserts
    // - How many asserts failed
    // - How many suites and tests had failed asserts

    int totalSuites = 0;
    int totalTests = 0;
    int totalAsserts = 0;
    int failSuites = 0;
    int failTests = 0;
    int failAsserts = 0;
    UDi_L_FOREACH((&UDi_reg), suites, UDi_suite, s) {
        totalSuites++;
        bool suiteFailed = false;
        UDi_L_FOREACH(s, tests, UDi_test, t) {
            totalTests++;
            bool testFailed = false;
            UDi_L_FOREACH(t, asserts, UDi_assert, a) {
                totalAsserts++;
                failAsserts++;
                testFailed = true;
                suiteFailed = true;
            }
            if (testFailed) {
                failTests++;
            }
        }
        if (suiteFailed) {
            failSuites++;
        }
    }
    int passTests = totalTests - failTests;

    printf("Passed %.01f%% of %d tests across %d suites.\n",
            100.0 * passTests / totalTests,
            totalTests, totalSuites);

    // Then we loop through again to print specific failure details.
    UDi_L_FOREACH((&UDi_reg), suites, UDi_suite, s) {
        bool suiteHead = false;
        UDi_L_FOREACH(s, tests, UDi_test, t) {
            bool testHead = false;
            UDi_L_FOREACH(t, asserts, UDi_assert, a) {
                if (!suiteHead) {
                    printf("Suite %s:\n", s->name);
                    suiteHead = true;
                }
                if (!testHead) {
                    printf("  Test %s:\n", t->name);
                    testHead = true;
                }
                printf("    Assert failed: %s\n", a->text);
                printf("      (at %s)\n", a->location);
            }
        }
    }

    UDi_free_suites();

    return failAsserts > 0 ? 1 : 0;
}

#undef UDi_L_FIELDS
#undef UDi_L_INIT
#undef UDi_L_ADD
#undef UDi_L_FOREACH

#define UDi_STR2(x) #x
#define UDi_STR(x) UDi_STR2(x)
#define UDi_LOCATION __FILE__ ":" UDi_STR(__LINE__)

#define UD_REGISTER_TESTS \
    void UDi_setup_tests(bool* succeeded)

#define UD_SUITE(name, init, clean) \
    UDi_suite* name##_suite; \
    do {\
        name##_suite = UDi_add_suite(#name, init, clean); \
        if (name##_suite == NULL) { \
            *succeeded = false; \
            return; \
        } \
    } while(0)

#define UD_TEST(suite, function) \
    do { \
        if (UDi_add_test(suite##_suite, #function, function) == NULL) { \
            *succeeded = false; \
            return; \
        } \
    } while (0)

#define UDA_FAIL(message) \
    UDi_add_assert(UDi_current_test, UDi_LOCATION, #message)

#define UDA_TRUE(cond) do { \
    if (!(cond)) { UDi_add_assert(UDi_current_test, UDi_LOCATION, #cond); } \
} while(0)

#define UDA_EQUAL(lhs, rhs) UDA_TRUE(lhs == rhs)
#define UDA_NOT_EQUAL(lhs, rhs) UDA_TRUE(lhs != rhs)
