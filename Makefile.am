-include Makefile.local

BUILT_SOURCES = gitrev.h

bin_PROGRAMS = sqlsmith

DUT = mysql.cc

sqlsmith_SOURCES = relmodel.cc schema.cc $(DUT)	 			\
    random.cc prod.cc expr.cc grammar.cc log.cc dump.cc impedance.cc	\
    sqlsmith.cc

sqlsmith_LDADD = $(BOOST_REGEX_LIB)

AM_LDFLAGS = $(BOOST_LDFLAGS)
AM_CPPFLAGS = $(BOOST_CPPFLAGS) -Wall -Wextra


EXTRA_DIST = gitrev.h dump.hh expr.hh grammar.hh log.hh prod.hh		\
    random.hh relmodel.hh schema.hh impedance.hh known.txt known_re.txt log.sql	\
    README.org TODO.org ast.png logo.png dump.xsl util.hh sqlite.hh	\
    dut.hh postgres.hh monetdb.hh mysql.hh log-v1.0-to-v1.2.sql

gitrev.h: $(HEADERS) $(SOURCES)
	-if git describe --dirty --tags --always > /dev/null ; then \
	echo "#define GITREV \"$$(git describe --dirty --tags --always)\"" > $@ ;\
	fi

