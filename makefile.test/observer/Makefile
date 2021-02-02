subdirs=$(shell ls -F | grep "\/" | sed 's/\///g')
cur_srcs=$(wildcard *.cpp)
cur_objs=$(patsubst %.cpp, %.o, $(cur_srcs))

all : $(subdirs) $(cur_objs)
$(subdirs) : ECHO
	make -C $@

$(cur_objs) : %.o : %.cpp
	$(CC) -c $^ -o $(root_dir)/$(obj_dir)/$@

ECHO:
	@echo $(subdirs)
