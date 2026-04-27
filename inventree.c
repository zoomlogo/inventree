/* Suckless inventory manager. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

enum operation { NONE, ADD, REMOVE, FIND, LIST, HISTORY };

struct item {
	char name[128];
	int count;

	int len;
	char names[16][64];
	int counts[16];

	char url[256];
};

struct item *
inv_get(char *file, size_t *r_len)
{
	FILE *fp = fopen(file, "rb");
	if (fp == NULL)
		return NULL;

	if (fseek(fp, 0, SEEK_END) != 0) {
		fclose(fp);
		return NULL;
	}

	size_t bz = ftell(fp);
	rewind(fp);

	char *buf = malloc(bz);
	fread(buf, 1, bz, fp);
	fclose(fp);

	*r_len = bz / sizeof(struct item);

	return (struct item *) buf;
}

enum argstate { NORM, ARGP_FILE, ARGP_COUNT };
#define ARG_IS(lit, slit) (!strcmp(arg, lit) || !strcmp(arg, slit))
int
main(int argc, char **argv)
{
	/* flags */
	char *file = NULL;
	enum operation op = NONE;
	int count = 1;
	bool all = false;
	bool person = false;
	bool item = false;

	enum argstate state = NORM;
	if (argc == 1) {
		fprintf(stderr, "usage: inventree -I filename [options]\n");
		fprintf(stderr, "check manpage for more info.\n");
		return -1;
	}

	/* parse args */
	for (int i = 1; i < argc; ++i) {
		char *arg = argv[i];
		switch (state) {
		case NORM:
			if (ARG_IS("-A", "-add"))
				op = ADD;
			else if (ARG_IS("-R", "-remove"))
				op = REMOVE;
			else if (ARG_IS("-F", "-find"))
				op = FIND;
			else if (ARG_IS("-H", "-history"))
				op = HISTORY;
			else if (ARG_IS("-L", "-list"))
				op = LIST;
			else if (ARG_IS("-I", "-inventory"))
				state = ARGP_FILE;
			else if (ARG_IS("-n", "-count"))
				state = ARGP_COUNT;
			else if (ARG_IS("-a", "-all"))
				all = true;
			else if (ARG_IS("-p", "-person"))
				person = true;
			else if (ARG_IS("-i", "-item"))
				item = true;
			break;
		case ARGP_FILE:
			file = arg;
			break;
		case ARGP_COUNT:
			count = atoi(arg);
		}
	}

	/* arg errors */


	/* inv manage */
	size_t len;
	struct item *items = inv_get(file, &len);

	inv_set(file, items);
	free(items);

	return 0;
}
