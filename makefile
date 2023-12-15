.PHONY: clean dist

name ?= unknown

dist: clean
	tar -hzcf "$(name).tar.gz" conf/* makefile

clean:
	$(MAKE) -C conf clean
