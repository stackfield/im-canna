all: textview entry entry2

textview: textview.c
	gcc -o textview textview.c `pkg-config gtk+-2.0 --cflags --libs`

entry: entry.c
	gcc -o entry entry.c `pkg-config gtk+-2.0 --cflags --libs`

entry2: entry2.c
	gcc -o entry2 entry2.c `pkg-config gtk+-2.0 --cflags --libs`

install: textview entry
	/usr/bin/install -c -s textview /usr/bin/textview
	/usr/bin/install -c -s entry /usr/bin/entry
