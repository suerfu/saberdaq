# Makefile for compiing saber library
#
# 1) The library consists of code for PMT waveform digitization via CAEN VME ( daq )
# 2) Preliminary analysis code
# 3) DAQ code for measuring coating thickness via laser attenuation
# 4) Coating monitor
# 5) Pressure and temperature measurement during powder drying procedure

CC = g++ -g # -pg # -g
	# compiler is g++ 


NAME = saber
LIBNAME = lib$(NAME).so
    # library name


PREFIX = /usr/local
    # library installation directory


DAQ_CPP_FILES = $(wildcard daq/src/*.cpp)
DAQ_OBJ_FILES = $(patsubst %.cpp, %.o, $(DAQ_CPP_FILES))
    # source and object files for data acquisition


        CAEN_FLAGS = -I$(CAEN_DIR)
        LD_CAEN = -lCAENVME
        # CAEN libraries

        DAQ_CPP_FILES = $(wildcard daq/src/*.cpp)
        DAQ_OBJ_FILES = $(patsubst %.cpp, %.o, $(DAQ_CPP_FILES))
            # source and object files for data acquisition
            # will be compiled only when CAEN libraries are installed

        EXE_FILES = bin/saber-update-format bin/saber-convert-hdf5
            # executables depend on ADC formats, etc., which depend on CAEN libraries
        ifdef ROOTSYS
            EXE_FILES = $(EXE_FILES) bin/saber-event-viewer bin/saber-spec-generator bin/saber-avg-waveform
        endif

    #endif


    SERIAL_CPP_FILES = $(wildcard serial/src/*.cpp)
    SERIAL_OBJ_FILES = $(patsubst %.cpp, %.o, $(SERIAL_CPP_FILES))
	    # source and object files for pyrolitic coating measurement
	
	AN_CPP_FILES = $(wildcard analysis/src/*.cpp)
	AN_OBJ_FILES = $(patsubst %.cpp, %.o, $(AN_CPP_FILES))
    # source and object files for analysis


PYROMAP_CPP_FILES = $(wildcard pyromap/src/*.cpp)
PYROMAP_OBJ_FILES = $(patsubst %.cpp, %.o, $(PYROMAP_CPP_FILES))
	# source and object files for pyrolitic coating measurement


COATING_CPP_FILES = $(wildcard coating/src/*.cpp)
COATING_OBJ_FILES = $(patsubst %.cpp, %.o, $(COATING_CPP_FILES))
	# source and object files for pyrolitic coating monitoring


DRYING_CPP_FILES = $(wildcard drying/src/*.cpp)
DRYING_OBJ_FILES = $(patsubst %.cpp, %.o, $(DRYING_CPP_FILES))
	# source and object files for drying monitoring


CFLAGS = -Wall -std=c++0x -fPIC -Ianalysis/include -Idaq/include -Ipyromap/include -Icoating/include -Idrying/include
	# turn on all warning; use c++ 11 stamdard, compile position independent code
	# include various directories


PLRS_FLAGS = -I/home/suerfu/polaris/include
LD_POLARIS = -L/home/suerfu/polaris/lib -lpolaris
    # polaris flags and linker


CAEN_FLAGS = -I/home/suerfu/caen/include
LD_CAEN = -L/home/suerfu/caen/CAENVMELib-2.50 -lCAENVME
    # CAEN libraries

HDF5_FLAGS = -I/mnt/saber/software/hdf5/include -I/usr/local/hdf5/include
LD_HDF5 = -L/mnt/saber/software/hdf5/lib -lhdf5 -lhdf5_hl -lhdf5_cpp -L/usr/local/hdf5/lib -lhdf5 -lhdf5_hl -lhdf5_cpp
    # HDF5 libraries

H5MAN_FLAGS = -Ih5manager/include

ifdef ROOTSYS
ROOT_FLAGS = `root-config --cflags`
LD_ROOT = `root-config --glibs`
    # ROOT for analysis and event display
endif


all : bin 
# lib


bin : bin/saber-convert-hdf5 
# bin/saber-event-viewer bin/saber-update-format bin/saber-spec-generator bin/saber-avg-waveform

bin/saber-convert-hdf5 : exe/saber-convert-hdf5.cpp $(DAQ_OBJ_FILES) $(AN_OBJ_FILES) h5manager/src/H5FileManager.o
	@echo "compiling $@"
	@$(CC) $(CFLAGS) $(PLRS_FLAGS) $(ROOT_FLAGS) $(HDF5_FLAGS) $(H5MAN_FLAGS) $^ -o $@ $(LD_POLARIS) $(LD_CAEN) $(LD_ROOT) $(LD_HDF5)

bin/saber-update-format : exe/saber-update-format.cpp $(DAQ_OBJ_FILES) $(AN_OBJ_FILES) 
	@echo "compiling $@"
	@$(CC) $(CFLAGS) $(PLRS_FLAGS) $(ROOT_FLAGS) $(HDF5_FLAGS) $(H5MAN_FLAGS) $^ -o $@ $(LD_POLARIS) $(LD_CAEN) $(LD_ROOT) $(LD_HDF5)

bin/saber-event-viewer : exe/saber-event-viewer.cpp $(DAQ_OBJ_FILES) $(AN_OBJ_FILES)
	@echo "compiling $@"
	@$(CC) $(CFLAGS) $(PLRS_FLAGS) $(ROOT_FLAGS) $(HDF5_FLAGS) $(H5MAN_FLAGS) $^ -o $@ $(LD_POLARIS) $(LD_CAEN) $(LD_ROOT) $(LD_HDF5)

bin/saber-spec-generator : exe/saber-spec-generator.cpp $(DAQ_OBJ_FILES) $(AN_OBJ_FILES)
	@echo "compiling $@"
	@$(CC) $(CFLAGS) $(PLRS_FLAGS) $(ROOT_FLAGS) $(HDF5_FLAGS) $(H5MAN_FLAGS) $^ -o $@ $(LD_POLARIS) $(LD_CAEN) $(LD_ROOT) $(LD_HDF5)

bin/saber-avg-waveform : exe/saber-avg-waveform.cpp $(DAQ_OBJ_FILES) $(AN_OBJ_FILES)
	@echo "compiling $@"
	@$(CC) $(CFLAGS) $(PLRS_FLAGS) $(ROOT_FLAGS) $(HDF5_FLAGS) $(H5MAN_FLAGS) $^ -o $@ $(LD_POLARIS) $(LD_CAEN) $(LD_ROOT) $(LD_HDF5)

# ==============================================================


lib : lib/$(LIBNAME)

lib/$(LIBNAME) : $(DAQ_OBJ_FILES) $(AN_OBJ_FILES) $(PYROMAP_OBJ_FILES) $(COATING_OBJ_FILES) $(DRYING_OBJ_FILES)
	@echo "linking $@"
	@mkdir -p lib
	@$(CC) -fPIC -shared -Wl,-soname,$(LIBNAME) -o $@ $^ ${LD_POLARIS} $(LD_CAEN) $(LD_ROOT)

%.o : %.cpp
	@echo "compiling $@"
	@$(CC) $(CFLAGS) $(PLRS_FLAGS) $(CAEN_FLAGS) $(ROOT_FLAGS) $(HDF5_FLAGS) $(H5MAN_FLAGS) -c $^ -o $@ 


install:
	@echo "installing SABRE libraries..."
	@mkdir -p ${PREFIX}/include/$(NAME)
	@cp -r analysis/include daq/include ${PREFIX}/include/$(NAME)
	@cp lib/$(LIBNAME) ${PREFIX}/lib/
	@cp bin/saber-event-viewer bin/saber-spec-generator bin/saber-update-format ${PREFIX}/bin/
	@sudo ldconfig -n ${PREFIX}/lib/lib${NAME}.so


uninstall:
	rm -rf ${PREFIX}/include/${NAME}
	rm ${PREFIX}/lib/$(LIBNAME)
	rm ${PREFIX}/bin/$(NAME)


clean:
	@echo "cleaning..."
	@-rm ${DAQ_OBJ_FILES} > /dev/null 2>&1
	@-rm ${AN_OBJ_FILES} > /dev/null 2>&1
	@-rm ${PYROMAP_OBJ_FILES} > /dev/null 2>&1
	@-rm ${COATING_OBJ_FILES} > /dev/null 2>&1
	@-rm ${DRYING_OBJ_FILES} > /dev/null 2>&1
	@-rm lib/${LIBNAME} > /dev/null 2>&1
	@-rm bin/* > /dev/null 2>&1

