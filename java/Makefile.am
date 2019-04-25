javadir = $(pkgdatadir)/lib

PKG = net/sourceforge/zbar
java_DATA = zbar.jar

java_LTLIBRARIES = libzbarjni.la
libzbarjni_la_CPPFLAGS = $(JAVA_CFLAGS) $(AM_CPPFLAGS)
libzbarjni_la_LIBADD = $(abs_top_builddir)/zbar/libzbar.la

libzbarjni_la_SOURCES = zbarjni.c zbarjni.h
BUILT_SOURCES = zbarjni.h
MAINTAINERCLEANFILES = zbarjni.h

zbar_jar_SRCS = \
    $(abs_srcdir)/$(PKG)/Config.java \
    $(abs_srcdir)/$(PKG)/Modifier.java \
    $(abs_srcdir)/$(PKG)/Orientation.java \
    $(abs_srcdir)/$(PKG)/Symbol.java \
    $(abs_srcdir)/$(PKG)/SymbolIterator.java  \
    $(abs_srcdir)/$(PKG)/SymbolSet.java \
    $(abs_srcdir)/$(PKG)/Image.java \
    $(abs_srcdir)/$(PKG)/ImageScanner.java

zbar_jar_CLASSES = $(zbar_jar_SRCS:.java=.class)

test_SRCS = test/TestImage.java test/TestImageScanner.java \
    test/TestScanImage.java
test_CLASSES = TestImage TestImageScanner TestScanImage

CLEANFILES = zbar.jar $(zbar_jar_CLASSES) $(test_CLASSES:=.class)

if HAVE_JAVAH

# Works up to Java 8
zbarjni.h: $(zbar_jar_SRCS) zbar.jar
	classes=`echo $(zbar_jar_CLASSES:.class=) | tr / .` ; \
		$(JAVAH) -o $@ $$classes

else

# Yeah, Java > 8 really messed up with headers generation

PKGH = ${shell echo ${PKG}|sed s,/,_,g}

# Should be in sync with the files at zbar_jar_SRCS with generate headers
HDRS = \
	$(abs_builddir)/$(PKGH)_ImageScanner.h \
	$(abs_builddir)/$(PKGH)_Symbol.h \
	$(abs_builddir)/$(PKGH)_SymbolSet.h

zbarjni.h: $(zbar_jar_SRCS)
	$(JAVAC) -h $(abs_builddir) $(zbar_jar_SRCS)
	cat $(HDRS) > $(abs_builddir)/zbarjni.h

CLEANFILES += $(HDRS)

endif

zbar.jar: $(zbar_jar_SRCS)
	cd $(abs_srcdir); $(JAVAC) -d $(abs_builddir) $(zbar_jar_SRCS)
	$(JAR) cf $@ $(zbar_jar_CLASSES) || $(RM) $@

#require junit java package
check-java: zbar.jar libzbarjni.la
	echo "making check in java"
	cd $(abs_srcdir); $(JAVAC) -classpath $(abs_builddir)/zbar.jar:.:$(CLASSPATH) -d $(abs_builddir) $(test_SRCS)
	$(abs_top_builddir)/libtool -dlopen $(abs_top_builddir)/zbar/libzbar.la -dlopen $(abs_builddir)/libzbarjni.la --mode=execute $(JAVA) -Xcheck:jni -classpath zbar.jar:.:$(CLASSPATH) org.junit.runner.JUnitCore $(test_CLASSES)
