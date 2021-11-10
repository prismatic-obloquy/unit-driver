// The one library all the unit tests are testing against. Split out so that
// the functionality doesn't clutter up the testing demo.

int add1(int num) {
    return num + 1;
}

// don't worry about what this function does, it just spins the CPU a bit
// (and a variable amount of time, so we can hit concurrency)
int super_complex(int input) {
    for (int i = 0; i < input % 50 + 50; ++i) {
        switch (i % 4) {
        case 0:
            for (int j = 0; i < input % 100000; ++j) {
                input += j;
            }
            break;
        case 1:
            input = (input * 25214903917 + 11) % 1 << 48;
            break;
        case 2:
            for (int j = 0; i < input % 25; ++j) {
                // might produce all zeroes
                input ^= input * 3;
            }
            break;
        case 3:
            input += i * 1024;
        }
    }
    return input;
}
