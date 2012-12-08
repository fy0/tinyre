
APPNAME = "tinyre"

CC   = gcc
LINK = link

bin = ../bin
obj = ../obj

VPATH+=$(bin)
VPATH+=$(obj)

CFLAGS         = -Wall -Wno-format #-std=c99 
LDFLAGS_LIB    = -shared -fpic
LDFLAGS_LIB_PY = -shared -I/usr/include/python2.7 -L/usr/bin -fpic -lpython2.7

objs        = $(patsubst %.c,%.o, $(wildcard *.c))
objs_lib    = $(patsubst %.c,%.lo, $(wildcard *.c))
objs_py_lib = $(patsubst %.c,%.lpo, $(wildcard *.c))

$(APPNAME)    : $(objs)
	mkdir -p $(bin)
	cd $(obj) && $(CC) $(CFLAGS) -o $(bin)/$(APPNAME) $(objs)

lib           : $(objs_lib)
	mkdir -p $(bin)
	cd $(obj) && $(CC) $(CFLAGS) $(LDFLAGS_LIB) -o $(bin)/$(APPNAME).so $(objs_lib)

pylib27       : $(objs_py_lib)
	@mkdir -p $(bin)
	cd $(obj) && $(CC) $(CFLAGS) $(LDFLAGS_LIB_PY) -o $(bin)/_tre.so $(objs_py_lib)

%.o           : %.c
	@mkdir -p $(obj)
	$(CC) -c $(CFLAGS) -g -D DEBUG -D DEMO -o $(obj)/$@ $<

%.lo         : %.c
	@mkdir -p $(obj)
	$(CC) -c $(CFLAGS) -fpic -D lib -o $(obj)/$@ $<

%.lpo         : %.c
	@mkdir -p $(obj)
	$(CC) -c $(CFLAGS) -fpic -D py_lib_27 -o $(obj)/$@ $<

run           : $(APPNAME)
	$(bin)/$(APPNAME)

clean         :
	rm $(obj)/*.o -f
	rm $(obj)/*.lo -f
	rm $(obj)/*.lpo -f
	rm $(bin)/_tre.so -f
	rm $(bin)/$(APPNAME) -f
	rm $(bin)/$(APPNAME).so -f

