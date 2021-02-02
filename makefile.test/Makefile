#set complier
CC = g++

#set directories
subdirs  = $(shell ls -F | grep "\/" | sed 's/\///g' | sed 's/target//g')
root_dir = $(shell pwd)
obj_dir  = target/obj
bin_dir  = target/bin

bin = myobservertest

cur_srcs = $(wildcard *.cpp)
cur_objs = $(patsubst *.cpp, *.o, $(cur_srcs))

export CC bin root_dir obj_dir bin_dir

.PHONY:all
all : $(subdirs)  TARGET

$(subdirs) : ECHO
#	@echo zhumengri 2
	make -C $@

TARGET : ECHO
#	@echo zhumengri 3
	make -C target

ECHO:
#	@echo zhumengri 1
	@echo $(subdirs) 

$(cur_objs) : %.o : %.cpp
	$(CC) -c $^ -o $(root_dir)/$(obj_dir)/$@



.PHONY:clean
cleanall: cleanobj
	-rm $(root_dir)/$(bin_dir)/$(bin)

cleanobj:
	-rm $(root_dir)/$(obj_dir)/*.o
