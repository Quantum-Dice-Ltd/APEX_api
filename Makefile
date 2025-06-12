MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables

LIB := \
	BUILT/qdapex_api.o

MINIPROG := \
	BUILT/qdapexmini.o

TESTPROG := \
	BUILT/util.o \
	BUILT/qdapextest.o

OBJS := $(LIB) $(MINIPROG) $(TESTPROG)

all: BUILT/qdapexmini BUILT/qdapextest

BUILT/qdapexmini: $(MINIPROG) $(LIB) | BUILT/
	./link $@ $(MINIPROG) $(LIB)

BUILT/qdapextest: $(TESTPROG) $(LIB) | BUILT/
	./link $@ $(TESTPROG) $(LIB)

# static rules

$(OBJS): BUILT/%.o: %.c ./compile | BUILT/
	./compile $@ $<

%/:
	mkdir -p $@

-include BUILT/*.d
