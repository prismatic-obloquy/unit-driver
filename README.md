# Unit Driver

Unit Driver is a single-header unit-testing framework for C.

There were basically two ways to unit-test C code:

- CUnit, which stopped being updated in 2014, and none of whose forks have achieved much popularity -- and most of which are dead too.
- GTest, which is updated and well-documented, and also designed for C++, which can cause problems, and just doesn't fit great.

Both are hard to integrate cross-plaform, because they're separate dependencies and need to be downloaded and/or installed.
Both are also somewhat clunky to use and integrate, though in return you get more control.

Unit Driver changes that, and makes it easier to write and orchestrate unit tests.
Just include a header file, and you get:

- A whole suite/test/assert infrastructure, like CUnit, with suite-level initialization and cleanup
- A prewritten `main` to orchestrate registered tests
- A variety of assertion macros which log details
- Effortless parallelization, if you want

And soon, you'll get:

- Fatal assert failures
- Function mocking
- Output format control

## Example

Unit Driver files tend to look like:

```c
#define UD_THREAD_COUNT 8
#include "unit-driver.h"

#include "my-system-header.h"

void* MyThing_init(void) {
  return mt_alloc();
}

void MyThing_clean(void* thing) {
  mt_free(thing);
}

void MyThing_number_is_42(const void* v_my_thing) {
  struct MyThing* my_thing = v_my_thing;
  int num = my_get_number(my_thing);
  UDA_EQUAL(num, 42);
}

UD_REGISTER_TESTS {
  UD_SUITE(NyThing_get_number, MyThing_init, MyThing_clean);
  UD_TEST(MyThing_get_number, MyThing_number_is_42);
}
```

And that's it.
That's your whole testing architecture.
No `main` needed; Unit Driver handles that.
All you need is that `#include` line, and the `UD_REGISTER_TESTS` block at the bottom.
When run, if the assert fails, you get a concise but detailed explanation of what happened:

```
Suite MyThing_get_number:
  Test MyThing_number_is_42:
    Assert failed: num == 42
      (at /home/deep-thought/uqc/src/test/MyThing.c:16)
```

It's not *nothing*, but it's pretty damn simple.

## Usage

The structure of Unit Driver-based unit testing is something like:

- You have one 'orchestrating' `.c` file, which `#include`s Unit Driver and contains your `UD_REGISTER_TESTS` block
- Each suite is another `.c` file, which is also `#include`d in the main one
- Suites cover one facet of the program -- e.g. a header file, some functions in the module that interact, etc.
- Each test handles one specific case, trying to be both valid and invalid in as many ways as possible
- You build your unit test executable with one compiler command, just targeting the main `.c` file
- Run the executable to run all your tests

There are a few preprocessor variables to configure `unit-driver.h`:

- `UD_THREAD_COUNT`: Pick how many threads to run.
  
  If you set it to `0`, or leave it unset, no threading at all is used.
  `1` means a single thread, and is kinda pointless.
  Anything more means that many threads will split up testing.

  Note that threading uses C11 standard threads, which may require extra configuration outside of Linux.

Each suite has an optional suite-level setup and teardown.
You can omit them -- pass `NULL` to `UD_SUITE` -- but you have to omit both, or neither.
The setup function returns a `void*`, which will then be passed to each test as a `const void*`, to discourage tests from modifying it.
That cleanup function gets the original `void*`, and needs to deallocate any allocated memory, close and opened file sockets, etc.
Bear in mind that the setup is run once per *suite*, not per test.

As implied, tests get the context produced by the setup function, or `NULL` if there wasn't one.
**Do not modify the context in tests.**
If you think you need to, you're writing your unit tests wrong!
First, there's no guaranteed order to the tests, and it'll be deliberately randomized soon.
Second, if tests are run in parallel, you could easily have data races and other undefined behavior.

## Interface and Versioning

Unit Driver defines quite a lot of symbols that you may be able to see from your source file, but only as implementation details.
The public symbols start with `UD_`, for "backbone" functionality like setting up tests.
The asserts all start with `UDA_`.
Everything else starts with `_UD_`, note the leading underscore, and you really, really shouldn't call those.

Unit Driver isn't really versioned.
The `unit-driver.h` on the `release` branch is, assuming I haven't messed up, always good, so just grab the latest and go.
It's expected you won't update it until you want new features, and when you do, you'll just grab the latest -- including, if necessary, porting your unit tests.
After all, they'll change semi-regularly anyway.

That said, the `UD_` and `UDA_` exports are changed as little as possible, to keep porting minimal.
`_UD_` can change whenever is convenient, which is part of why you shouldn't use it.
