
APPNAME = "tinyre"

CC   = gcc
LINK = link

bin = ../bin
obj = ../obj

VPATH+=$(bin)
VPATH+=$(obj)

CFLAGS = -g -std=c99 -Wall -Wno-format 
# LDFLAGS = 

src     = $(wildcard *.c)
objects = $(patsubst %.c,%.o,$(src))

$(APPNAME)    : $(objects)
	mkdir -p $(bin)
	cd $(obj) && $(CC) $(CFLAGS) -o $(bin)/$(APPNAME) $(objects)

$(objects)    : $(src)
	mkdir -p $(obj)
	$(CC) -c $(CFLAGS) -o $(obj)/$@ $(patsubst %.o,%.c,$@)

run           : $(APPNAME)
	$(bin)/$(APPNAME)

clean         :
	rm $(obj)/*.o
	rm $(bin)/$(APPNAME)

