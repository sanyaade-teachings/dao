
PLATS = linux macosx freebsd mingw minix

$(PLATS) :
	cd tools/daomake && make $@
	./tools/daomake/daomake --platform $@
	make -f Makefile.daomake