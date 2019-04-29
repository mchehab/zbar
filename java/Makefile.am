javadir = $(pkgdatadir)/lib

PKG = net/sourceforge/zbar
java_DATA = zbar.jar

java_LTLIBRARIES = libzbarjni.la
libzbarjni_la_CPPFLAGS = $(JAVA_CFLAGS) $(AM_CPPFLAGS)
libzbarjni_la_LIBADD = $(abs_top_builddir)/zbar/libzbar.la

nodist_libzbarjni_la_SOURCES = zbarjni.h
libzbarjni_la_SOURCES = zbarjni.c $(nodist_libzbarjni_la_SOURCES)
BUILT_SOURCES = $(nodist_libzbarjni_la_SOURCES)
MAINTAINERCLEANFILES = $(nodist_libzbarjni_la_SOURCES)

zbar_jar_SRCS = \
    $(PKG)/Config.java $(PKG)/Modifier.java $(PKG)/Orientation.java \
    $(PKG)/Symbol.java $(PKG)/SymbolIterator.java $(PKG)/SymbolSet.java \
    $(PKG)/Image.java $(PKG)/ImageScanner.java

zbar_jar_CLASSES = $(zbar_jar_SRCS:.java=.class)

test_SRCS = test/TestImage.java test/TestImageScanner.java \
    test/TestScanImage.java
test_CLASSES = TestImage TestImageScanner TestScanImage

EXTRA_DIST = $(zbar_jar_SRCS) $(test_SRCS)

CLEANFILES = zbar.jar $(nodist_libzbarjni_la_SOURCES) $(zbar_jar_CLASSES) $(test_CLASSES:=.class)

if HAVE_JAVAH

# Works up to Java 8
zbarjni.h: $(zbar_jar_SRCS) zbar.jar
	classes=`echo $(zbar_jar_CLASSES:.class=) | tr / .` ; \
		$(JAVAH) -o $@ $$classes

else

# After Java 8, it is not possible anymore to build single zbarjni.h
# As we don't want to break ABI, we need to join several .h files into one

PKGH = ${shell echo ${PKG}|sed s,/,_,g}

zbarjni.h: $(zbar_jar_SRCS)
	$(JAVAC) -h $(abs_builddir) $(abs_srcdir)/$(PKG)/*.java
	cat $(abs_builddir)/$(PKGH)_*.h > $(abs_builddir)/zbarjni.h
	rm $(abs_builddir)/$(PKGH)_*.h

endif

zbar.jar: $(zbar_jar_SRCS)
	cd $(abs_srcdir); $(JAVAC) -d $(abs_builddir) $(zbar_jar_SRCS)
	$(JAR) cf $@ $(zbar_jar_CLASSES) || $(RM) $@

#require junit java package
check-java: zbar.jar libzbarjni.la
	echo "making check in java"
	cd $(abs_srcdir); $(JAVAC) -classpath $(abs_builddir)/zbar.jar:.:$(CLASSPATH) -d $(abs_builddir) $(test_SRCS)
	$(abs_top_builddir)/libtool -dlopen $(abs_top_builddir)/zbar/libzbar.la -dlopen $(abs_builddir)/libzbarjni.la --mode=execute $(JAVA) -Xcheck:jni -classpath zbar.jar:.:$(CLASSPATH) org.junit.runner.JUnitCore $(test_CLASSES)
