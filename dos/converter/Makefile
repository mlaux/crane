CC=gcc
CFLAGS=-Wall -O2

json2crane: json2crane.c cJSON.c
	$(CC) $(CFLAGS) -o json2crane json2crane.c cJSON.c -lm

clean:
	rm -f json2crane
