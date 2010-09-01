# set default debug
ifndef debug
  debug = 1 
endif


CXX = g++
CFLAGS =
CXXFLAGS := ${CFLAGS}

    ifeq (${debug}, 0)
        OPTS = -O3 -w
    else
        OPTS = -g #-Wall
    endif
    AR = ar rc 

  DEPS = -MMD -MF $*.d

include ./Makefile.srpp_defaults

SRPP_INC = -I src/libSRPP/include \
		   -I include \
		   -I /usr/local/include/jrtplib3/ \
		   -I /usr/include/qt4/Qt \

DEFS +=  
INCL = -I. ${SRPP_INC}  
LIBS = ${SRPP_LIB} 

default_target: sqrkal

objs: ${SRPP_OBJS} 

libs: ${SRPP_LIBFILE} 

${SRPP_LIBFILE}: ${SRPP_OBJS}
	${AR} ${SRPP_LIBFILE} ${SRPP_OBJS}
	
sqrkal: SQRKal.cpp ${SRPP_LIBFILE} 
	${CXX} ${CXXFLAGS} ${OPTS} ${DEFS} ${INCL} $< ${LIBS} -o sqrkal

%.o: %.cpp
	${CXX} -c ${CXXFLAGS} ${OPTS} ${DEFS} ${INCL} ${DEPS} $< -o $@
	cat $*.d >> Dependencies
	rm -f $*.d

# clean targets
CLEAN = ${SRPP_LIBFILE} sqrkal 

clean:
	rm -f `find . -name '*.o'` 
	rm -f `find . -name '*.d'`
	rm -f Dependencies
	touch Dependencies
	rm ${CLEAN}

# include automatically generated dependencies
-include Dependencies