TARGET : demo

CXXFLAGS=-c -std=c++11 -I..
CXXFLAGS+=-O2
LIBS+=-lstdc++

OBJS_PATH=objs
SRCS=sample
OBJS=$(foreach f,${SRCS},$(OBJS_PATH)/$(basename $f).o)

$(OBJS_PATH)/%.o : %.cpp ${wildcard ../*.h} Makefile | $(OBJS_PATH)
	@echo Compiling $@
	@$(CC) $(CXXFLAGS) $< -o $@

demo : ${OBJS}
	@echo Linking $@
	@$(CC) $+ $(LIBS) -o $@

$(OBJS_PATH) :
	@echo Creating temporal folder
	@mkdir $(OBJS_PATH)

clean :
	rm -f objs/*
	rm -f app
