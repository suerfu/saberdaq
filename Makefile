# Makefile for compiing saber library
#
# 1) The library consists of code for PMT waveform digitization via CAEN VME ( daq )
# 2) Preliminary analysis code
# 3) DAQ code for measuring coating thickness via laser attenuation
# 4) Coating monitor
# 5) Pressure and temperature measurement during powder drying procedure

CC = g++ -g # -pg # -g
        # compiler is g++


NAME = saberdaq
LIBNAME = lib$(NAME).so
    # library name


PREFIX = /usr
    # library installation directory


CPP_FILES = $(wildcard src/*.cpp analysis/src/*.cpp h5manager/src/*.cpp)
OBJ_FILES = $(patsubst %.cpp, %.o, $(CPP_FILES))
    # source and object files for data acquisition


CFLAGS = -Wall -std=c++0x -fPIC -Ianalysis/include -Iinclude -Ih5manager/include

# polaris flags and linker
CFLAGS += -I/usr/local/include/polaris
LDFLAGS = -L/usr/local/lib -lpolaris


# CAEN libraries
CFLAGS += -I/usr/include/
LDFLAGS += -L/usr/lib/libCAENVME.so -lCAENVME

# HDF5 libraries
CFLAGS += -I/usr/local/HDF_Group/HDF5/1.14.6/include
LDFLAGS += -L/usr/local/HDF_Group/HDF5/1.14.6/lib -lhdf5 -lhdf5_hl -lhdf5_cpp

EXE_FILES = bin/saber-update-format bin/saber-convert-hdf5
# executables depend on ADC formats, etc., which depend on CAEN libraries
ifdef ROOTSYS
        EXE_FILES = $(EXE_FILES) bin/saber-event-viewer bin/saber-spec-generator bin/saber-avg-waveform
endif


ifdef ROOTSYS
CFLAGS += `root-config --cflags`
LDFLAGS += `root-config --glibs`
    # ROOT for analysis and event display
endif


all : lib bin

bin : #bin/saber-convert-hdf5 bin/saber-event-viewer bin/saber-update-format #bin/saber-spec-generator bin/saber-avg-waveform

bin/saber-convert-hdf5 : exe/saber-convert-hdf5.cpp $(OBJ_FILES)
	@echo "compiling $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

bin/saber-update-format : exe/saber-update-format.cpp $(OBJ_FILES)
	@echo "compiling $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

bin/saber-event-viewer : exe/saber-event-viewer.cpp $(OBJ_FILES)
	@echo "compiling $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

bin/saber-spec-generator : exe/saber-spec-generator.cpp $(OBJ_FILES)
	@echo "compiling $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

bin/saber-avg-waveform : exe/saber-avg-waveform.cpp $(OBJ_FILES)
	@echo "compiling $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# ==============================================================


lib : lib/$(LIBNAME)

lib/$(LIBNAME) : $(OBJ_FILES)
	@echo "linking $@"
	@mkdir -p lib
	@$(CC) -fPIC -shared -Wl,-soname,$(LIBNAME) -o $@ $^ ${LDFLAGS}

%.o : %.cpp
	@echo "compiling $@"
	@$(CC) $(CFLAGS) -c $^ -o $@


install:
	@echo "installing SABRE libraries..."
	@mkdir -p ${PREFIX}/include/$(NAME)
	@cp -r analysis/include include ${PREFIX}/include/$(NAME)
	@cp lib/$(LIBNAME) ${PREFIX}/lib/
	@cp bin/saber-event-viewer bin/saber-spec-generator bin/saber-update-format ${PREFIX}/bin/
	@sudo ldconfig -n ${PREFIX}/lib/lib${NAME}.so


uninstall:
	rm -rf ${PREFIX}/include/${NAME}
	rm ${PREFIX}/lib/$(LIBNAME)
	rm ${PREFIX}/bin/$(NAME)


clean:
	@echo "cleaning..."
	@-rm ${OBJ_FILES} > /dev/null 2>&1
	@-rm lib/${LIBNAME} > /dev/null 2>&1
	@-rm bin/* > /dev/null 2>&1
