
.PHONY: data logic setup

all: setup data logic
	cp $(CURDIR)/static/*.* $(CURDIR)/bin/

setup:
	mkdir -p $(CURDIR)/bin

data: setup
	$(MAKE) -C $(CURDIR)/data

	# payload and definition
	cp $(CURDIR)/data/data.bin $(CURDIR)/bin/code.0008.bin
	bin2codedef $(CURDIR)/bin/code.0008.bin 00090000 $(CURDIR)/bin/code.0008.def 004A7D98

logic: setup
	$(MAKE) -C $(CURDIR)/logic

	# payload and definition
	cp $(CURDIR)/logic/logic.bin $(CURDIR)/bin/code.0009.bin
	bin2codedef $(CURDIR)/bin/code.0009.bin 01E60000 $(CURDIR)/bin/code.0009.def 004A7D98

clean:
	$(MAKE) -C $(CURDIR)/logic clean
	$(MAKE) -C $(CURDIR)/data clean
	#rm -rf $(CURDIR)/bin
