
.PHONY: data logic setup

all: setup logic data
	cp $(CURDIR)/static/*.* $(CURDIR)/bin/

setup:
	mkdir -p $(CURDIR)/bin

logic: setup
	$(MAKE) -C $(CURDIR)/logic


	# payload and definition
	cp $(CURDIR)/logic/logic.bin $(CURDIR)/bin/code.0009.bin
	bin2codedef $(CURDIR)/bin/code.0009.bin 0062F028 $(CURDIR)/bin/code.0009.def 004A7D98


data: setup
	$(MAKE) -C $(CURDIR)/data

	# payload and definition
	cp $(CURDIR)/data/data.bin $(CURDIR)/bin/code.0008.bin
	bin2codedef $(CURDIR)/bin/code.0008.bin 00760000 $(CURDIR)/bin/code.0008.def 004A7D98

clean:
	$(MAKE) -C $(CURDIR)/logic clean
	$(MAKE) -C $(CURDIR)/data clean
	rm -rf $(CURDIR)/bin
