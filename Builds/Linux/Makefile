ZMQ_INCDIR:=/usr/local/include
ZMQ_LIBDIR:=/usr/local/lib
ZMQ_RTLIBDIR:=$(ZMQ_LIBDIR)

LIBNAME := $(notdir $(CURDIR))
OBJDIR := $(OBJDIR)/$(LIBNAME)
TARGET := $(LIBNAME).so
OS := $(shell uname)

SRC_DIR := ${shell find ./ -type d -print}
VPATH := $(SOURCE_DIRS)

SRC := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
OBJ := $(addprefix $(OBJDIR)/,$(notdir $(SRC:.cpp=.o)))



#Extra macros and libraries needed by the plugin
CXXFLAGS := $(CXXFLAGS)  -I $(ZMQ_INCDIR)
LDFLAGS := $(LDFLAGS) -lzmq -L$(ZMQ_LIBDIR) -Wl,-rpath=$(ZMQ_RTLIBDIR)


BLDCMD := $(CXX) -shared -o $(OUTDIR)/$(TARGET) $(OBJ) $(LDFLAGS) $(RESOURCES) $(TARGET_ARCH)

VPATH = $(SRC_DIR)

.PHONY: objdir

$(OUTDIR)/$(TARGET): objdir $(OBJ)
	-@mkdir -p $(BINDIR)
	-@mkdir -p $(LIBDIR)
	-@mkdir -p $(OUTDIR)
	@echo "Building $(TARGET)"
	@$(BLDCMD)

$(OBJDIR)/%.o : %.cpp
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) -o "$@" -c "$<"
	
	
objdir:
	-@mkdir -p $(OBJDIR)

clean:
	@echo "Cleaning $(LIBNAME)"
	-@rm -rf $(OBJDIR)
	-@rm -f $(OUTDIR)/$(TARGET)

-include $(OBJ:%.o=%.d)
