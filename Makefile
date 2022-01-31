CFLAGS=-g -Wall -O3 -fPIC
LDFLAGS=-lcrypto `curl-config --libs`

OS=$(shell lsb_release -si)
ifeq ($(OS),Ubuntu)
#	Ubuntu
	INSTDIR=/lib/x86_64-linux-gnu/security
else
#	Fedora
	INSTDIR=/usr/lib64/security
endif
ETCDIR=/etc/security

SRCS=src/cas.c src/url.c src/ini.c src/config.c src/dit_upm.c src/pam_cas.c
TESTS=src/test.c src/test-pt.c
HDRS=src/cas.h src/url.h src/ini.h src/config.h src/dit_upm.h

module: $(SRCS) $(HDRS)
	mkdir -p obj
	gcc $(CFLAGS) -c src/pam_cas.c -o obj/pam_cas.o
	gcc $(CFLAGS) -c src/cas.c -o obj/cas.o
	gcc $(CFLAGS) -c src/url.c  -o obj/url.o
	gcc $(CFLAGS) -c src/ini.c -o obj/ini.o
	gcc $(CFLAGS) -c src/config.c -o obj/config.o
	gcc $(CFLAGS) -c src/dit_upm.c -o obj/dit_upm.o
	gcc -shared -o obj/pam_cas.so obj/dit_upm.o obj/pam_cas.o obj/cas.o obj/url.o obj/ini.o obj/config.o  $(LDFLAGS)

tests: $(TESTS) $(HDRS)
	mkdir -p obj
	gcc $(CFLAGS) -c src/test.c -o obj/test.o
	gcc $(CFLAGS) -c src/test-pt.c -o obj/test-pt.o
	gcc $(CFLAGS) -c src/cas.c -o obj/cas.o
	gcc $(CFLAGS) -c src/url.c  -o obj/url.o
	gcc $(CFLAGS) -c src/ini.c -o obj/ini.o
	gcc $(CFLAGS) -c src/config.c -o obj/config.o
	gcc $(CFLAGS) -c src/dit_upm.c -o obj/dit_upm.o
	gcc $(CFLAGS) $(LDFLAGS) -o obj/test obj/dit_upm.o obj/test.o obj/cas.o obj/url.o obj/ini.o obj/config.o $(LDFLAGS)
	gcc $(CFLAGS) $(LDFLAGS) -o obj/test-pt obj/dit_upm.o obj/test-pt.o obj/cas.o obj/url.o obj/ini.o obj/config.o $(LDFLAGS)

all: module tests

dist: all README Changelog conf/common-auth.sample
	mkdir -p dist$(INSTDIR)
	mkdir -p dist$(ETCDIR)
	mkdir -p dist/usr/local/bin
	mkdir -p dist/usr/share/doc/pam_cas
	cp obj/test dist/usr/local/bin/test
	cp obj/test-pt dist/usr/local/bin/test-pt
	cp obj/pam_cas.so dist$(INSTDIR)/pam_cas.so
	cp conf/pam_cas.conf dist$(ETCDIR)/pam_cas.conf
	cp README dist/usr/share/doc/pam_cas/README
	cp Changelog dist/usr/share/doc/pam_cas/Changelog
	cp conf/common-auth.sample dist/usr/share/doc/common-auth.sample

distfile: dist
	tar zcvf pam_cas-reloaded.tgz -C dist .

# must be done as sudo
install: pam_cas-reloaded.tgz
	mv $(ETCDIR)/pam_cas.conf $(ETCDIR)/pam_cas.conf.orig || true
	tar zxvf pam_cas-reloaded.tgz -C /

clean:
	rm obj/*.o obj/*.so obj/test obj/test-pt

distclean: clean
	rm -rf dist pam_cas-reloaded.tgz
  