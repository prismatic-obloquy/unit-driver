# Unit Driver

Unit Driver is a single-header unit-testing framework for C.

There were basically two ways to unit-test C code:

- CUnit, which stopped being updated in 2014, and none of whose forks have achieved much popularity -- and most of which are dead too.
- GTest, which is updated and well-documented, and also designed for C++, which can cause problems, and just doesn't fit great.

Both are hard to integrate cross-plaform, because they're separate dependencies and need to be downloaded and/or installed.
Both are also somewhat clunky to use and integrate, though in return you get more control.

Unit Driver changes that, and makes it easier to write and orchestrate unit tests.
Just include a header file, and you get:

- A while suite/test/assert infrastructure, like CUnit
- A prewritten `main` to orchestrate registered tests
- A variety of assertion macros which log details
- Effortless parallelization

And soon, you'll get:

- Fatal assert failures
- Function mocking
- Output format control

Unit Driver files tend to look like:

```c
#define UD_THREAD_COUNT 8
#include "unit-driver.h"

#include "my-system-header.h"

void* init_MyThing(void) {
  return mt_alloc();
}

void clean_MyThing(void* thing) {
  mt_free(thing);
}

void MyThing_number_is_42(const void* v_my_thing) {
  struct MyThing* my_thing = v_my_thing;
  int num = my_get_number(my_thing);
  UDA_EQUAL(num, 42);
}

UD_REGISTER_TESTS {
  UD_SUITE(NyThing_get_number, init_MyThing, clean_MyThing);
  UD_TEST(MyThing_get_number, MyThing_number_is_42);
}
```

And that's it.
That's your whole file.
No `main` needed; Unit Driver handles that.
All you need is that `#include` line, and the `UD_REGISTER_TESTS` block at the bottom.
When run, if the assert fails, you get a concise but detailed explanation of what happened:

```
Suite MyThing_get_number:
  Test MyThing_number_is_42:
    Assert failed: num == 42
      (at /home/deep-thought/uqc/src/test/MyThing.c:16)
```

And all you had to do was download the `.h`, then `#include` it.
