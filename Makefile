Bin: ; mkdir -p $@
Deps: ; mkdir -p $@
DEBUG=
OPT=-O3

DEPFILES:= $(wildcard Deps/*.dep)
include $(DEPFILES)

Bin/roll: roll/roll.c | Deps Bin
	$(CC) $< -o $@ -MT $@ -MD -MP -MF Deps/roll.dep $(OPT) $(DEBUG)

README.html: README.md README.css
	pandoc README.md README.css -f markdown -o $@ -s --toc

.PHONY: clean
clean:
	rm -rf Bin/*
.PHONY: all
all: Bin/roll

.DEFAULT_GOAL:=all
