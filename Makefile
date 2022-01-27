CFLAGS=-g -Wall -O3 -fPIC
LDFLAGS=`curl-config --libs`

OS=$(shell lsb_release -si)
ifeq ($(OS),Ubuntu)
#	Ubuntu
	INSTDIR=/lib/x86_64-linux-gnu/security
else
#	Fedora
	INSTDIR=/usr/lib64/security
endif
ETCDIR=/etc/security

all: src/*.c src/*.h
	gcc $(CFLAGS) -c src/pam_cas.c -o obj/pam_cas.o
	gcc $(CFLAGS) -c src/cas.c -o obj/cas.o
	gcc $(CFLAGS) -c src/url.c  -o obj/url.o
	gcc $(CFLAGS) -c src/ini.c -o obj/ini.o
	gcc $(CFLAGS) -c src/config.c -o obj/config.o
	gcc -shared -o obj/pam_cas.so obj/pam_cas.o obj/cas.o obj/url.o obj/ini.o obj/config.o $(LDFLAGS)

test: src/test.c src/test-pt.c
	gcc $(CFLAGS) -c src/test.c -o obj/test.o
	gcc $(CFLAGS) -c src/test-pt.c -o obj/test-pt.o
	gcc $(CFLAGS) -c src/cas.c -o obj/cas.o
	gcc $(CFLAGS) -c src/url.c  -o obj/url.o
	gcc $(CFLAGS) -c src/ini.c -o obj/ini.o
	gcc $(CFLAGS) -c src/config.c -o obj/config.o
	gcc $(CFLAGS) $(LDFLAGS) -o obj/test obj/test.o obj/cas.o obj/url.o obj/ini.o obj/config.o
	gcc $(CFLAGS) $(LDFLAGS) -o obj/test-pt obj/test-pt.o obj/cas.o obj/url.o obj/ini.o obj/config.o

dist: all test
	mkdir -p dist$(INSTDIR)
	mkdir -p dist$(ETCDIR)
	mkdir -p dist/usr/local/bin
	mkdir -p dist/usr/share/doc/pam_cas
	cp obj/test dist/usr/local/bin/test
	cp obj/test-pt dist/usr/local/bin/test-pt
	cp obj/pam_cas.so dist$(INSTDIR)/pam_cas.so
	cp conf/pam_cas.conf dist$(ETCDIR)/pam_cas.conf
	cp README dist/usr/share/doc/pam_cas/README
	tar zcvf pam_cas-reloaded.tgz -C dist .

# must be done as sudo
install: dist
	tar zxvf pam_cas-reloaded.tgz -C /

clean:
	rm obj/*.o obj/*.so obj/test obj/test-pt

distclean: clean
	rm -rf dist pam_cas-reloaded.tgz
  