all: checkmakefiles
	cd src && $(MAKE)

clean: checkmakefiles
	cd src && $(MAKE) clean

cleanall: checkmakefiles
	cd src && $(MAKE) MODE=release clean
	cd src && $(MAKE) MODE=debug clean
	rm -f src/Makefile

# Location of the INET framework. LabSCim links against libINET and imports its NED and
# .msg definitions, so opp_makemake has to be told where INET lives -- a bare
# "opp_makemake -f --deep" produces a Makefile that fails at
# "cannot resolve import 'inet.common.INETDefs'".
#
# Defaults to the workspace layout (<root>/inet, two levels up from models/labscim).
# Override for any other arrangement:
#     make makefiles INET_PROJ=/path/to/inet
INET_PROJ ?= $(realpath ../../inet)

makefiles:
	@if [ -z "$(INET_PROJ)" ] || [ ! -d "$(INET_PROJ)/src" ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'INET not found. Expected <workspace>/inet, or pass it explicitly:'; \
	echo '    make makefiles INET_PROJ=/path/to/inet'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
	fi
	cd src && opp_makemake -f --deep \
	    -KINET_PROJ=$(INET_PROJ) -DINET_IMPORT \
	    -I$(INET_PROJ)/src -L$(INET_PROJ)/src \
	    -lboost_system -lcryptopp -lpthread -lrt -lINET

checkmakefiles:
	@if [ ! -f src/Makefile ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'src/Makefile does not exist. Please use "make makefiles" to generate it!'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
	fi
