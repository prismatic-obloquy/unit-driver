# Examples

In this subdirectory are a handful of examples showing how you can use Unit Driver.
They also serve as tests, because they're easy to compile and run and compare against the expected output.

Any of them can be compiled with more or less any C compiler, and it shouldn't change anything unless that compiler doesn't support that example's requirements.
However, tests are specifically built with Clang on Ubuntu 20.04, so that should always work.
You can see the exact command in `.github/workflows/test.yml`.
Eventually I'll get around to automating Windows and Mac testing too, but if you want it sooner, you're welcome to submit a pull request.
And, of course, if you can't get it to work, raise an issue.

Any example-specific requirements are documented at the top of the example itself.

If you're unable to compile any of the examples, first double-check that you've actually got everything set up right and you're targeting the right example, then raise an issue.
