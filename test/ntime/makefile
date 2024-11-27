TARGET = test1
CXX = g++
LDFLAGS = -pthread -L../.. -lnlib
SRCDIR = .
INCDIR = $(SRCDIR) -I ../..
OBJDIR = obj


# uncomment the appropriate CFLAGS below to select build version
# debug build
CFLAGS= -ggdb -I$(INCDIR) -DDEBUG
# release build
# CFLAGS= -O3 -I$(INCDIR)

OBJS = $(OBJDIR)/$(TARGET).o

# all: objpath $(TARGET)
all: objpath $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.cxx
	$(CXX) -c -o $@ $^ $(CFLAGS)

objpath:
	mkdir -p $(OBJDIR)

$(TARGET):   $(OBJS)
	$(CXX) -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)
