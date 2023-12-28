.PHONY: clean dist

name ?= unknown

dist: clean
	tar -hzcf "$(name).tar.gz" conf/* analyse/* client/* da/* server/* shm_fifo/* utilitaire/* qlunch.conf qlunch.conf.d makefile

clean:
	$(MAKE) -C conf clean
	$(MAKE) -C client clean
	$(MAKE) -C server clean
	$(MAKE) -C shm_fifo clean
