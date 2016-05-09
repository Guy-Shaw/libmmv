CC = gcc

PROGRAM := qp-encode-file
SRCS := $(PROGRAM).c
OBJS := $(PROGRAM).o libmmv/libmmv.a
CONFIG :=
CFLAGS = -I./inc -g -Wall

.PHONY: all

all:	$(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(CONFIG) $(OBJS)

case: cscope.out TAGS

cscope.out:
	rm -f cscope.out cscope.files cscope.in.out cscope.po.out ; cscope-init

TAGS:
	rm -f TAGS
	etags *.[ch]

clean-test:
	rm -f test_?? T.??

clean:
	rm -f core a.out mmv *.o *.a
	rm -f test_?? T.??
	rm -f *,FAILED

realclean: clean
	rm -f cscope.out cscope.files cscope.in.out cscope.po.out TAGS tags
	rm -rf tmp

#END
