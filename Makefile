.phony all:
all: ACS

ACS: acs.c linked_list.c
	gcc -pthread acs.c linked_list.c -o ACS

.PHONY clean:
clean:
	-rm -rf *.o *.exe