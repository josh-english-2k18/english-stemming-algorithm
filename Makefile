CC ?= cc
CXX ?= c++
FUZZ_CC ?= clang
AR ?= ar
PYTHON ?= python3
BUILD_DIR ?= build
CPPFLAGS ?=
CFLAGS ?= -O3
CXXFLAGS ?= -O3
LDFLAGS ?=
LDLIBS ?=
WARNINGS = -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes -Werror
C_COMPILE = $(CC) $(CPPFLAGS) -I. -Itests -std=c11 $(WARNINGS) $(CFLAGS)
SANITIZERS = -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined

.PHONY: all example test check sanitize thread-sanitize coverage benchmark fuzz fuzz-smoke tooling-test clean
all: $(BUILD_DIR)/libesa.a $(BUILD_DIR)/esaExample

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/esa.o: esa.c esa.h esa_rules.h | $(BUILD_DIR)
	$(C_COMPILE) -c esa.c -o $@

$(BUILD_DIR)/libesa.a: $(BUILD_DIR)/esa.o
	$(AR) rcs $@ $<

$(BUILD_DIR)/esaExample: esaExample.c $(BUILD_DIR)/libesa.a esa.h
	$(C_COMPILE) esaExample.c $(BUILD_DIR)/libesa.a $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/test_esa: tests/test_esa.c tests/reference.c tests/reference.h esa.h esa_rules.h $(BUILD_DIR)/libesa.a
	$(C_COMPILE) tests/test_esa.c tests/reference.c $(BUILD_DIR)/libesa.a $(LDFLAGS) -pthread $(LDLIBS) -o $@

$(BUILD_DIR)/test_oom: tests/test_oom.c esa.c esa.h esa_rules.h | $(BUILD_DIR)
	$(C_COMPILE) tests/test_oom.c $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/test_cpp: tests/test_cpp.cpp esa.h esa_rules.h $(BUILD_DIR)/libesa.a
	$(CXX) $(CPPFLAGS) -I. -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror $(CXXFLAGS) tests/test_cpp.cpp $(BUILD_DIR)/libesa.a $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/fuzz_smoke: tests/fuzz_smoke.c tests/fuzz_esa.c tests/reference.c tests/reference.h esa.h esa_rules.h $(BUILD_DIR)/libesa.a
	$(C_COMPILE) tests/fuzz_smoke.c tests/fuzz_esa.c tests/reference.c $(BUILD_DIR)/libesa.a $(LDFLAGS) $(LDLIBS) -o $@

example: $(BUILD_DIR)/esaExample
	$(BUILD_DIR)/esaExample

test: $(BUILD_DIR)/test_esa $(BUILD_DIR)/test_oom $(BUILD_DIR)/test_cpp $(BUILD_DIR)/fuzz_smoke
	$(BUILD_DIR)/test_esa
	$(BUILD_DIR)/test_oom
	$(BUILD_DIR)/test_cpp
	$(BUILD_DIR)/fuzz_smoke

check: all test

tooling-test:
	CC="$(CC)" $(PYTHON) -m unittest discover -s tests -p 'test_*.py' -v

sanitize:
	$(MAKE) BUILD_DIR=build/sanitize CFLAGS="$(SANITIZERS)" CXXFLAGS="$(SANITIZERS)" LDFLAGS="-fsanitize=address,undefined" test

thread-sanitize:
	$(MAKE) BUILD_DIR=build/thread-sanitize CFLAGS="-g -O1 -fsanitize=thread" CXXFLAGS="-g -O1 -fsanitize=thread" LDFLAGS="-fsanitize=thread" test

coverage:
	$(PYTHON) tools/coverage.py

benchmark:
	CC="$(CC)" $(PYTHON) benchmarks/run.py --output build/benchmark.json

fuzz-smoke: $(BUILD_DIR)/fuzz_smoke
	$(BUILD_DIR)/fuzz_smoke

fuzz: | $(BUILD_DIR)
	$(FUZZ_CC) $(CPPFLAGS) -I. -Itests -std=c11 -g -O1 -fsanitize=fuzzer,address,undefined esa.c tests/reference.c tests/fuzz_esa.c -o $(BUILD_DIR)/fuzz_esa
	mkdir -p $(BUILD_DIR)/fuzz-corpus
	$(BUILD_DIR)/fuzz_esa -max_total_time=20 -max_len=512 $(BUILD_DIR)/fuzz-corpus

clean:
	rm -rf build
