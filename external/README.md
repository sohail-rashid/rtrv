# External dependencies directory

This directory should contain git submodules for external libraries:

- `googletest/` - Google Test framework for unit testing
- `benchmark/` - Google Benchmark framework for performance testing

## Setup

Initialize submodules with:

```bash
git submodule add https://github.com/google/googletest.git external/googletest
git submodule add https://github.com/google/benchmark.git external/benchmark
git submodule update --init --recursive
```
