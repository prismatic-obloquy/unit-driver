/***************************************************************************\
|                          NOTE TO DEVELOPERS                               |
+---------------------------------------------------------------------------+
| This is Unit Driver. It just orchestrates the tests. You shouldn't edit   |
| it unless you're changing *how* tests work. If you just want to add or    |
| remove tests, see all-tests.c. To change them, look for the relevant call |
| to UD_SUITE and UD_TEST, then edit the functions that those use. Don't    |
| edit this file unless you know what you're doing.                         |
|                                                                           |
| See github.com/prismatic-obloquy/unit-driver/blob/release/README.md#usage |
| for usage information and guidance.                                       |
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

#ifndef _UD_PREVIOUSLY_INCLUDED
#define _UD_PREVIOUSLY_INCLUDED

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

#define _UD_L_FIELDS(elem_t, name) \
    elem_t* name##_ary; int name##_cap; int name##_len

#define _UD_L_INIT(name) \
    .name##_ary = NULL, .name##_cap = 0, .name##_len = 0

#define _UD_L_ADD(cont, name, elem, success) do { \
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

#define _UD_L_FOREACH(cont, name, elem_t, elem) \
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
} _UD_assert;

typedef struct {
    const char* name;
    UD_test_f run;

    _UD_L_FIELDS(_UD_assert, asserts);
} _UD_test;

typedef struct {
    const char* name;
    UD_init_f init;
    UD_clean_f clean;

    _UD_L_FIELDS(_UD_test, tests);
} _UD_suite;

typedef struct {
    _UD_L_FIELDS(_UD_suite, suites);
} _UD_registry;

static _UD_registry _UD_reg = {
    _UD_L_INIT(suites),
};

_UD_suite* _UD_add_suite(const char* name, UD_init_f init, UD_clean_f clean) {
    if ((init == NULL) ^ (clean == NULL)) {
        // either init is null or clean is null but not both
        return NULL;
    }
    _UD_suite new = {
        .name = name,
        .init = init,
        .clean = clean,
        _UD_L_INIT(tests),
    };
    bool succeeded;
    _UD_L_ADD((&_UD_reg), suites, new, succeeded);
    if (!succeeded) {
        return NULL;
    } else {
        return &_UD_reg.suites_ary[_UD_reg.suites_len-1];
    }
}

_UD_test* _UD_add_test(_UD_suite* s, const char* name, UD_test_f run) {
    _UD_test new = {
        .name = name,
        .run = run,
        _UD_L_INIT(asserts),
    };
    bool succeeded;
    _UD_L_ADD(s, tests, new, succeeded);
    if (!succeeded) {
        return NULL;
    } else {
        return &s->tests_ary[s->tests_len-1];
    }
}

_UD_assert* _UD_add_assert(
    _UD_test* t, const char* location,
    const char* text
) {
    _UD_assert new = {
        .location = location,
        .text = text,
    };
    bool succeeded;
    _UD_L_ADD(t, asserts, new, succeeded);
    if (!succeeded) {
        return NULL;
    } else {
        return &t->asserts_ary[t->asserts_len-1];
    }
}

void _UD_free_suites(void) {
    _UD_L_FOREACH((&_UD_reg), suites, _UD_suite, s) {
        _UD_L_FOREACH(s, tests, _UD_test, t) {
            // nothing to free in each assert, so just free the whole array
            free(t->asserts_ary);
        }
        free(s->tests_ary);
    }
    free(_UD_reg.suites_ary);
    // only resetting the outer layer because the rest are inaccessible once
    // it's nulled out
    _UD_reg = (_UD_registry) {
        _UD_L_INIT(suites),
    };
}

const char* _UD_asprintf(const char* fmt, ...) {
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

void _UD_setup_tests(bool*);

#if UD_THREAD_COUNT > 0
thread_local
#endif
_UD_test* _UD_current_test;
void _UD_run_test(_UD_test* test, const void* data) {
    _UD_current_test = test;
    test->run(data);
}

#if UD_THREAD_COUNT > 0
typedef struct {
    _Atomic bool running;
    struct {
        mtx_t lock;
        _UD_test* test;
        const void* data;
    } items[UD_THREAD_COUNT * 2];
} _UD_queue;

bool _UD_next_test(_UD_queue* queue, _UD_test** o_test, const void** o_data) {
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

int _UD_pull_tests(void* v_queue) {
    _UD_queue* queue = v_queue;
    _UD_test* test;
    const void* data;
    while (queue->running) {
        if (!_UD_next_test(queue, &test, &data)) {
            thrd_sleep(&(struct timespec) { .tv_nsec = 5000000 }, NULL);
            continue;
        }
        printf("Found test %s; running\n", test->name);
        _UD_run_test(test, data);
    }
    // once the queue is stopped, empty it before finishing up
    while (_UD_next_test(queue, &test, &data)) {
        printf("Found test %s; running\n", test->name);
        _UD_run_test(test, data);
    }
    return 0;
}
#endif

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;
    // this is slightly awkward, but means we don't need a `return` at the
    // end of the _UD_setup_tests definition, which is convenient.
    bool succeeded = true;
    _UD_setup_tests(&succeeded);
    if (!succeeded) {
        puts("Failed to set up tests");
        _UD_free_suites();
        return 1;
    }

#if UD_THREAD_COUNT > 0
    // Generate all the data first
    void* datae[_UD_reg.suites_len]; // plural of the plural :)
    for (int i = 0; i < _UD_reg.suites_len; ++i) {
        if (_UD_reg.suites_ary[i].init != NULL) {
            datae[i] = _UD_reg.suites_ary[i].init();
        } else {
            datae[i] = NULL;
        }
    }
    thrd_t threads[UD_THREAD_COUNT];
    _UD_queue queue = {
        .running = true,
    };
    for (int i = 0; i < UD_THREAD_COUNT * 2; ++i) {
        mtx_init(&queue.items[i].lock, mtx_plain);
        queue.items[i].test = NULL;
        queue.items[i].data = NULL;
    }
    for (int i = 0; i < UD_THREAD_COUNT; ++i) {
        thrd_create(&threads[i], _UD_pull_tests, &queue);
    }
    for (int i = 0; i < _UD_reg.suites_len; ++i) {
        _UD_L_FOREACH((&_UD_reg.suites_ary[i]), tests, _UD_test, t) {
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
    for (int i = 0; i < _UD_reg.suites_len; ++i) {
        for (int i = 0; i < _UD_reg.suites_len; ++i) {
            if (_UD_reg.suites_ary[i].clean != NULL) {
                _UD_reg.suites_ary[i].clean(datae[i]);
                datae[i] = NULL;
            }
        }
    }
#else
    _UD_L_FOREACH((&_UD_reg), suites, _UD_suite, s) {
        void* data = s->init ? s->init() : NULL;
        _UD_L_FOREACH(s, tests, _UD_test, t) {
            _UD_run_test(t, data);
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
    _UD_L_FOREACH((&_UD_reg), suites, _UD_suite, s) {
        totalSuites++;
        bool suiteFailed = false;
        _UD_L_FOREACH(s, tests, _UD_test, t) {
            totalTests++;
            bool testFailed = false;
            _UD_L_FOREACH(t, asserts, _UD_assert, a) {
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
    _UD_L_FOREACH((&_UD_reg), suites, _UD_suite, s) {
        bool suiteHead = false;
        _UD_L_FOREACH(s, tests, _UD_test, t) {
            bool testHead = false;
            _UD_L_FOREACH(t, asserts, _UD_assert, a) {
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

    _UD_free_suites();

    return failAsserts > 0 ? 1 : 0;
}

#undef _UD_L_FIELDS
#undef _UD_L_INIT
#undef _UD_L_ADD
#undef _UD_L_FOREACH

#define _UD_STR2(x) #x
#define _UD_STR(x) _UD_STR2(x)
#define _UD_LOCATION __FILE__ ":" _UD_STR(__LINE__)

#define UD_REGISTER_TESTS \
    void _UD_setup_tests(bool* succeeded)

#define UD_SUITE(name, init, clean) \
    _UD_suite* name##_suite; \
    do {\
        name##_suite = _UD_add_suite(#name, init, clean); \
        if (name##_suite == NULL) { \
            *succeeded = false; \
            return; \
        } \
    } while(0)

#define UD_TEST(suite, function) \
    do { \
        if (_UD_add_test(suite##_suite, #function, function) == NULL) { \
            *succeeded = false; \
            return; \
        } \
    } while (0)

#define UDA_FAIL(message) \
    _UD_add_assert(_UD_current_test, _UD_LOCATION, #message)

#define UDA_TRUE(cond) do { \
    if (!(cond)) { _UD_add_assert(_UD_current_test, _UD_LOCATION, #cond); } \
} while(0)

#define UDA_EQUAL(lhs, rhs) UDA_TRUE(lhs == rhs)
#define UDA_NOT_EQUAL(lhs, rhs) UDA_TRUE(lhs != rhs)

#endif
