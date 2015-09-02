CFLAGS=-g -Wall -O3 -fPIC
LDFLAGS=`curl-config --libs`

all:
	gcc $(CFLAGS) -c pam_cas.c
	gcc $(CFLAGS) -c cas.c
	gcc $(CFLAGS) -c url.c 
	gcc $(CFLAGS) -c ini.c
	gcc $(CFLAGS) -c config.c
	gcc -shared  -o pam_cas.so pam_cas.o cas.o url.o ini.o config.o $(LDFLAGS)
test:
	gcc $(CFLAGS) -c test.c
	gcc $(CFLAGS) -c test-pt.c
	gcc $(CFLAGS) -c cas.c
	gcc $(CFLAGS) -c url.c
	gcc $(CFLAGS) -c ini.c
	gcc $(CFLAGS) -c config.c
	gcc $(CFLAGS) $(LDFLAGS) -o test test.o cas.o url.o ini.o config.o
	gcc $(CFLAGS) $(LDFLAGS) -o test-pt test-pt.o cas.o url.o ini.o config.o
install:
	cp pam_cas.so /lib/x86_64-linux-gnu/security/

clean:
	rm *.o *.so test test-pt
  