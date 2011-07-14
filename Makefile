# change these to reflect your Lua installation
LUA= /usr
LUAINC= $(LUA)/include
LUALIB= $(LUA)/lib
LUABIN= $(LUA)/bin

# no need to change anything below here
CFLAGS= $(INCS) $(DEFS) $(WARN) -O2 -fPIC
WARN= -Wall
INCS= -I$(LUAINC) -I/usr/include/libxml2/
LIBS= -lxml2

OBJS= lua-libxml2.o

SOS= libxml2.so

all: $(SOS)

$(SOS): $(OBJS)
	$(CC) -o $@ -shared $(OBJS) $(LIBS)

.PHONY: clean doc
clean:
	rm -f $(OBJS) $(SOS) core core.* a.out


tar: clean
	git archive --format=tar --prefix=lua-xmlreader-$(VERSION)/ v$(VERSION) | gzip > lua-xmlreader-$(VERSION).tar.gz
