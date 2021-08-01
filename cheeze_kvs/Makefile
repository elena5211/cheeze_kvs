OBJDIR = ./obj

all:
	cd $(OBJDIR) && make

deps:
	cd ./util/libtree && make
	cd ./util/lfqueue/ && rm -rf build/* && mkdir -p build
	cd ./util/lfqueue/build/ && cmake .. && make

clean:
	cd $(OBJDIR) && make clean

depend:
	cd $(OBJDIR) && make depend
