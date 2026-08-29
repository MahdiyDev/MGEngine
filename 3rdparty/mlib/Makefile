# Aggregate Makefile -- recurse into every module.
#
#   make               run every module's test suite   (default)
#   make examples      build and run every example
#   make clean         clean every module
#   make vec           just one module's tests
#                      (also: string list stream hashmap async)
#   make -k test       keep going after a failing module
#
# Each module has its own Makefile; this only recurses. On Windows use
# `mingw32-make`. builder/ is intentionally excluded -- it still depends on the
# removed dynamic_array/ headers.

MODULES := vec string list stream hashmap async

.PHONY: all test examples clean $(MODULES)

all: test

test:
	$(foreach m,$(MODULES),$(MAKE) -C $(m) test &&) echo all module tests passed

examples:
	$(foreach m,$(MODULES),$(MAKE) -C $(m) examples &&) echo all module examples built

clean:
	$(foreach m,$(MODULES),$(MAKE) -C $(m) clean &&) echo cleaned

# `make vec` -> run just that module's tests
$(MODULES):
	$(MAKE) -C $@ test
