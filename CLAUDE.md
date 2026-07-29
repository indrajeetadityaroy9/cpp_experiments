# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Layout

A collection of independent C++23 experiments, one per subdirectory (`duan_sssp`, `modular_checksum`, `robinhood_hashtable`). Each has its own Makefile that includes the shared `common.mk` at the repo root. There is no top-level build — always `cd` into a project directory to build.

`common.mk` defines the shared toolchain settings: `-std=c++23 -Wall -Wextra -Werror`, `-O3` by default (`CXX ?= g++`, overridable). Projects override locally: `duan_sssp` uses `-O2`, `modular_checksum` adds `-march=native`.

Catch2 (amalgamated) is vendored in `vendor/` and compiled directly into each test binary — there is no external dependency or package manager.

## Commands

In any project directory:

```bash
make            # build main target(s)
make test       # build and run tests
make benchmark  # build and run benchmarks (where present; duan_sssp calls it `make complexity`)
make clean
```

`robinhood_hashtable` has no tests — only `make bench` / `make run`.

### Running a single test

Test binaries are Catch2 executables, so they accept name and tag filters:

```bash
cd modular_checksum && make sum_test && ./sum_test "Checksum matches naive implementation"   # by test name
./sum_test "[checksum]"   # by tag
```

In `duan_sssp`, each component has its own test binary (`test_partial_order_ds`, `test_find_pivots`, `test_base_case`, `test_bmssp`, `test_complexity`); build one with `make <name>` and run it directly.

## Architecture Notes

- **duan_sssp**: Implementation of the Duan et al. deterministic SSSP algorithm, split into `include/duan_sssp.hpp` (declarations, graph types, params) and `src/duan_sssp.cpp`. Tests in `tests/` map one-to-one to the paper's components: PartialOrderDS (Lemma 3.1), FindPivots (Alg. 1), BaseCase (Alg. 2), BMSSP (Alg. 3). Known state per its README: individual components pass their unit tests, but full-algorithm integration is incorrect on larger graphs (path graphs n > 10, grids, sparse random graphs). The paper PDF is in the directory for reference.
- **robinhood_hashtable** is header-only (`robin_hood.h`); the benchmark is a separate .cpp file.
- `duan_sssp` uses `std::expected` for error handling instead of exceptions — follow that pattern when extending it.
