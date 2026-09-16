# Contributing

This is a small embedded library. The useful contributions are:

- failing tests for a recovery case that currently mis-scans indexes
- a real SPI-NOR backend example (STM32 / ESP / nRF) that is still readable
- API hardening (`malloc`-free init, error-returning flash callbacks) without breaking recovery

## Build and test

Same as CI:

```sh
cmake -S gtest -B gtest/build -DCMAKE_BUILD_TYPE=Release
cmake --build gtest/build --config Release
ctest --test-dir gtest/build -C Release --output-on-failure --rerun-failed
```

Host walkthrough (no hardware, C only):

```sh
cmake -S . -B build
cmake --build build
./build/host_demo
```

Do not consider a recovery change done until `ctest` is green. Overflow + reboot cases in `gtest/test.cpp` are the regression net.

## Geometry to use in new tests

Keep `sector_size % slot_size == 0` and `slot_size >= 2`. Overflow recovery uses empty-slot geometry and the status byte, not payload. Complementary bit patterns (`0xA5` / `0x5A`) catch missing erase-ahead on NOR AND.

## Style

Match the existing C99 in `Src/` and `Inc/`. Public functions keep the trailing `_` suffix.
