
all: base

.PHONY: test
test:
	$(MAKE) -f Makefile.base check-size

stats:
	$(MAKE) -f Makefile.base stats

comp:
	$(MAKE) -f Makefile.comp

base:
	$(MAKE) -f Makefile.base

clean:
	rm -f *.irx *.o
