all:
	$(MAKE) -C llvm-backend
	$(MAKE) -C vm

clean:
	$(MAKE) -C llvm-backend clean
	$(MAKE) -C vm clean

check:
	./run-tests

.PHONY: all clean check
