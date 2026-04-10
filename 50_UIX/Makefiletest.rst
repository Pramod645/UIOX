TARGET_EXEC := final_program

BUILD_DIR := ./build
SRC_DIRS := ./UIXLib

# Find all the C and C++ files we want to compile
# Note the single quotes around the * expressions. The shell will incorrectly expand these otherwise, but we want to send the * directly to the find command.
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')

# Prepends BUILD_DIR and appends .o to every src file
# As an example, ./your_dir/hello.cpp turns into ./build/./your_dir/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) -MMD -MP

# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Build step for C++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)

/////////////
CC=arm-linux-gnueabihf-gcc
OPENMP=TRUE

ifeq ($(OPENMP),TRUE)
CFLAGS=-O2 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -fopenmp -fPIC -DLINUX -DOPENMP
EXECUTABLE=libldw-mp.so
else
CFLAGS=-O2 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -DLINUX -fPIC
EXECUTABLE=libldw.so
endif

#CFLAGS=-O2 -DLINUX -fPIC

LDFLAGS=-shared
SOURCES=LDW_ImageROI.c LDW_LaneExtraction.c LDW_LineDetection.c LDW_PreProcessing.c LDW_WarningGeneration.c LDW_InitlizeSettingParameters.c LDW_LaneValidation.c LDW_Main.c
OBJECTS=$(SOURCES:.cpp=.o)

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $(LOADLIBS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $(LOADLIBS) $< -o $@

clean: 
	rm -rf *o $(EXECUTABLE)

# /////////////////////
Implicit Rules
Make loves c compilation. And every time it expresses its love, things get confusing. 
Perhaps the most confusing part of Make is the magic/automatic rules that are made. 
Make calls these "implicit" rules. I don't personally agree with this design decision, 
and I don't recommend using them, but they're often used and are thus useful to know. 
Here's a list of implicit rules:

Compiling a C program: n.o is made automatically from n.c with a command of the form 
$(CC) -c $(CPPFLAGS) $(CFLAGS) $^ -o $@

Compiling a C++ program: n.o is made automatically from n.cc or n.cpp with a command of the form 
$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

Linking a single object file: n is made automatically from n.o by running the command 
$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@


The important variables used by implicit rules are:

CC: Program for compiling C programs; default cc
CXX: Program for compiling C++ programs; default g++
CFLAGS: Extra flags to give to the C compiler
CXXFLAGS: Extra flags to give to the C++ compiler
CPPFLAGS: Extra flags to give to the C preprocessor
LDFLAGS: Extra flags to give to compilers when they are supposed to invoke the linker	

# /////////////////////////
# This is a makefile intended for compiling simple C++ projects.
# It assumes that you have one or more source files in the main directory,
#    each of which contains an "int main()".
# There may also be a "src" directory, containing additional source files.
# All include files will be automatically listed as dependencies.

# Any folders starting with "lib" will be compiled into shared libraries.
# libMyLibrary should contain libMyLibrary/src and libMyLibrary/include
# -IlibMyLibrary/include will be added to the compiler flags
# All .cc files in libMyLibrary/src will be compiled into the shared library.
# The library will be created as lib/libMyLibrary.so



# Default build variables, can be overridden by command line options.

CXX      = g++
AR       = ar
CPPFLAGS =
CXXFLAGS = -g -O3
LDFLAGS  =
LDLIBS   =
RM       = rm -f
BUILD    = default

BUILD_SHARED = 1
BUILD_STATIC = 2

ifneq ($(BUILD),default)
    include build-targets/$(BUILD).inc
endif

# Additional flags that are necessary to compile.
# Even if not specified on the command line, these should be present.

override CPPFLAGS += -Iinclude
override CXXFLAGS +=
override LDFLAGS  += -Llib -Wl,-rpath,\$$ORIGIN/../lib -Wl,--no-as-needed
override LDLIBS   +=

# EVERYTHING PAST HERE SHOULD WORK AUTOMATICALLY

.SECONDARY:
.SECONDEXPANSION:
.PHONY: all clean force

include PrettyPrint.inc

# Find the source files that will be used.
EXE_SRC_FILES = $(wildcard *.cc)
EXECUTABLES = $(patsubst %.cc,bin/%,$(EXE_SRC_FILES))
SRC_FILES = $(wildcard src/*.cc)
O_FILES = $(patsubst %.cc,build/$(BUILD)/build/%.o,$(SRC_FILES))

# Find each library to be made.
LIBRARY_FOLDERS   = $(wildcard lib?*)
LIBRARY_INCLUDES  = $(patsubst %,-I%/include,$(LIBRARY_FOLDERS))
override CPPFLAGS += $(LIBRARY_INCLUDES)
LIBRARY_FLAGS     = $(patsubst lib%,-l%,$(LIBRARY_FOLDERS))
override LDLIBS   += $(LIBRARY_FLAGS)
library_src_files = $(wildcard lib$(1)/src/*.cc)
library_o_files   = $(patsubst %.cc,build/$(BUILD)/build/%.o,$(call library_src_files,$(1)))
library_os_files  = $(addsuffix s,$(call library_o_files,$(1)))

ifneq ($(BUILD_SHARED),0)
    SHARED_LIBRARY_OUTPUT = $(patsubst %,lib/%.so,$(LIBRARY_FOLDERS))
endif

ifneq ($(BUILD_STATIC),0)
    STATIC_LIBRARY_OUTPUT = $(patsubst %,lib/%.a,$(LIBRARY_FOLDERS))
endif

all: $(EXECUTABLES) $(SHARED_LIBRARY_OUTPUT) $(STATIC_LIBRARY_OUTPUT)
	@printf "%b" "$(DGREEN)Compilation successful$(NO_COLOR)\n"

# Update dependencies with each compilation
override CPPFLAGS += -MMD
-include $(shell find build -name "*.d" 2> /dev/null)

.build-target: force
	@echo $(BUILD) | cmp -s - $@ || echo $(BUILD) > $@

bin/%: build/$(BUILD)/bin/% .build-target
	@mkdir -p $(@D)
	@$(call run_and_test,cp -f $< $@,Copying  )

lib/%: build/$(BUILD)/lib/% .build-target
	@mkdir -p $(@D)
	@$(call run_and_test,cp -f $< $@,Copying  )

ifeq ($(shell test $(BUILD_SHARED) -gt $(BUILD_STATIC); echo $$?),0)
build/$(BUILD)/bin/%: build/$(BUILD)/build/%.o $(O_FILES) | $(SHARED_LIBRARY_OUTPUT)
	@mkdir -p $(@D)
	@$(call run_and_test,$(CXX) $(ALL_LDFLAGS) $^ $(ALL_LDLIBS) -o $@,Linking  )
else
build/$(BUILD)/bin/%: build/$(BUILD)/build/%.o $(O_FILES) $(STATIC_LIBRARY_OUTPUT)
	@mkdir -p $(@D)
	@$(call run_and_test,$(CXX) $(ALL_LDFLAGS) $^ $(ALL_LDLIBS) -o $@,Linking  )
endif

build/$(BUILD)/bin/%: build/$(BUILD)/build/%.o $(O_FILES) | $(LIBRARY_OUTPUT)
	@mkdir -p $(@D)
	@$(call run_and_test,$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@,Linking  )

build/$(BUILD)/build/%.os: %.cc
	@mkdir -p $(@D)
	@$(call run_and_test,$(CXX) -c -fPIC $(CPPFLAGS) $(CXXFLAGS) $< -o $@,Compiling)

build/$(BUILD)/build/%.o: %.cc
	@mkdir -p $(@D)
	@$(call run_and_test,$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@,Compiling)


define library_variables
CPPFLAGS_EXTRA =
CXXFLAGS_EXTRA =
LDFLAGS_EXTRA  =
SHARED_LDLIBS  =
-include $(1)/Makefile.inc
build/$(1)/%.o: override CPPFLAGS := $$(CPPFLAGS) $$(CPPFLAGS_EXTRA)
build/$(1)/%.o: override CXXFLAGS := $$(CXXFLAGS) $$(CXXFLAGS_EXTRA)
lib/$(1).so:  override LDFLAGS  := $$(LDFLAGS)  $$(LDFLAGS_EXTRA)
lib/$(1).so:  override SHARED_LDLIBS := $$(SHARED_LDLIBS)
endef

$(foreach lib,$(LIBRARY_FOLDERS),$(eval $(call library_variables,$(lib))))

build/$(BUILD)/lib/lib%.a: $$(call library_o_files,%)
	@mkdir -p $(@D)
	@$(call run_and_test,$(AR) rcs $@ $^,Linking  )

build/$(BUILD)/lib/lib%.so: $$(call library_os_files,%)
	@mkdir -p $(@D)
	@$(call run_and_test,$(CXX) $(LDFLAGS) $^ -shared $(SHARED_LDLIBS) -o $@,Linking  )

clean:
	@printf "%b" "$(DYELLOW)Cleaning$(NO_COLOR)\n"
	@$(RM) -r bin build lib .build-target
///////////
All of our previous examples have used only shared libraries. While shared libraries are very useful, static libraries also have their use. Link-time optimization can be done much more aggressively. Any functions that are not called can be pruned by the compiler, whereas they cannot be pruned from a shared library. Here, we will improve the makefile to compile either shared or static libraries.

First, let’s add variables to indicate whether we should build a static or a shared library.

BUILD_SHARED = 1
BUILD_STATIC = 0
If BUILD_SHARED is non-zero, we will build a shared library. If BUILD_STATIC is non-zero, we will build a static library. If both are non-zero, we will make both libraries. We will link the executables against whichever library’s value is greater.

Now, we define each library that we want to make.

ifneq ($(BUILD_SHARED),0)
    SHARED_LIBRARY_OUTPUT = $(patsubst %,lib/%.so,$(LIBRARY_FOLDERS))
endif

ifneq ($(BUILD_STATIC),0)
    STATIC_LIBRARY_OUTPUT = $(patsubst %,lib/%.a,$(LIBRARY_FOLDERS))
endif
Next, we want to define a different rule for building the object files. When compiling a shared library, we want the -fPIC flag, whereas we don’t want it when compiling a static library. The object files to go into a static library will end in .o, and object files to go into a shared library will end in .os.

build/$(BUILD)/build/%.os: %.cc
	mkdir -p $(@D)
	$(CXX) -c -fPIC $(CPPFLAGS) $(CXXFLAGS) $< -o $@

build/$(BUILD)/build/%.o: %.cc
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
Now, we add a rule for each of the libraries. We will add a variable AR to allow the user to select which archiver to use. In addition, we define library_os_files in terms of library_o_files.

AR = ar
library_os_files  = $(addsuffix s,$(call library_o_files,$(1)))

build/$(BUILD)/lib/lib%.a: $$(call library_o_files,%)
	mkdir -p $(@D)
	$(AR) rcs $@ $^

build/$(BUILD)/lib/lib%.so: $$(call library_os_files,%)
	mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ -shared $(SHARED_LDLIBS) -o $@
Finally, we want to have a rule to compile the executables. We will make two versions of the rule, depending on whether we are linking against the shared or the static libraries.

ifeq ($(shell test $(BUILD_SHARED) -gt $(BUILD_STATIC); echo $$?),0)
build/$(BUILD)/bin/%: build/$(BUILD)/build/%.o $(O_FILES) | $(SHARED_LIBRARY_OUTPUT)
	mkdir -p $(@D)
	$(CXX) $(ALL_LDFLAGS) $^ $(ALL_LDLIBS) -o $@
else
build/$(BUILD)/bin/%: build/$(BUILD)/build/%.o $(O_FILES) $(STATIC_LIBRARY_OUTPUT)
	mkdir -p $(@D)
	$(CXX) $(ALL_LDFLAGS) $^ $(ALL_LDLIBS) -o $@
endif
Now, we can compile the code to either a shared or a static library, depending on what is needed for a particular project. The full makefile can be found on github.	


//
BUILD_SHARED = 0
BUILD_STATIC = 1

ifneq ($(BUILD_SHARED),0)
    SHARED_LIBRARY_OUTPUT = $(patsubst %,lib/%.so,$(LIBRARY_FOLDERS))
endif

ifneq ($(BUILD_STATIC),0)
    STATIC_LIBRARY_OUTPUT = $(patsubst %,lib/%.a,$(LIBRARY_FOLDERS))
endif

build/$(BUILD)/build/%.os: %.c
	mkdir -p $(@D)
	$(CC) -c -fPIC $(CPPFLAGS) $(CXXFLAGS) $< -o $@

build/$(BUILD)/build/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

AR = ar
library_os_files  = $(addsuffix s,$(call library_o_files,$(1)))

build/$(BUILD)/lib/lib%.a: $$(call library_o_files,%)
	mkdir -p $(@D)
	$(AR) rcs $@ $^

build/$(BUILD)/lib/lib%.so: $$(call library_os_files,%)
	mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ -shared $(SHARED_LDLIBS) -o $@	

ifeq ($(shell test $(BUILD_SHARED) -gt $(BUILD_STATIC); echo $$?),0)
build/$(BUILD)/bin/%: build/$(BUILD)/build/%.o $(O_FILES) | $(SHARED_LIBRARY_OUTPUT)
	mkdir -p $(@D)
	$(CXX) $(ALL_LDFLAGS) $^ $(ALL_LDLIBS) -o $@
else
build/$(BUILD)/bin/%: build/$(BUILD)/build/%.o $(O_FILES) $(STATIC_LIBRARY_OUTPUT)
	mkdir -p $(@D)
	$(CXX) $(ALL_LDFLAGS) $^ $(ALL_LDLIBS) -o $@
endif