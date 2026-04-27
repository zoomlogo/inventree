/* Suckless inventory manager. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ERR(msg) fprintf(stderr, msg "\n")

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

bool
inv_set(char *file, struct item *items, size_t len)
{
	FILE *fp = fopen(file, "wb");
	if (fp == NULL)
		return false;

	size_t bz = len * sizeof(struct item);
	size_t wr = fwrite((char *) items, 1, bz, fp);
	fclose(fp);

	return bz == wr;
}

bool
inv_add(struct item **items, size_t *i_len, struct item it)
{
	/* first search for existing item */
	char *nm = it.name;
	for (struct item *i = *items; i < *items + *i_len; ++i) {
		if (strcmp(i->name, nm) == 0) {
			i->count++;
			return true;
		}
	}

	struct item *np = realloc(*items, (*i_len + 1) * sizeof(struct item));
	if (np == NULL)
		return false;

	np[*i_len] = it;
	*i_len += 1;
	*items = np;
	return true;
}

void
inv_list(struct item *items, size_t len)
{
	printf("NAME\tCOUNT\tLINK\n");
	for (size_t i = 0; i < len; ++i) {
		struct item it = items[i];
		printf("%s\t%d\t%s\n", it.name, it.count, it.url);
	}
}

enum argstate { NORM, ARGP_FILE, ARGP_COUNT, ARGP_ADD_NM, ARGP_URL };
#define ARG_IS(lit, slit) (!strcmp(arg, lit) || !strcmp(arg, slit))
int
main(int argc, char **argv)
{
	/* flags */
	char *file = NULL;
	enum operation op = NONE;
	int count = 1;
	char *name = NULL;
	char *url = NULL;
	bool all = false;
	bool person = false;
	bool item = false;

	enum argstate state = NORM;
	if (argc == 1) {
		ERR("usage: inventree -I filename [options]");
		ERR("check manpage for more info.");
		return -1;
	}

	/* parse args */
	for (int i = 1; i < argc; ++i) {
		char *arg = argv[i];
		switch (state) {
		case NORM:
			if (ARG_IS("-A", "-add")) {
				op = ADD;
				state = ARGP_ADD_NM;
			} else if (ARG_IS("-R", "-remove"))
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
			else if (ARG_IS("-l", "-link"))
				state = ARGP_URL;
			else if (ARG_IS("-a", "-all"))
				all = true;
			else if (ARG_IS("-p", "-person"))
				person = true;
			else if (ARG_IS("-i", "-item"))
				item = true;
			break;
		case ARGP_FILE:
			file = arg;
			state = NORM;
			break;
		case ARGP_COUNT:
			count = atoi(arg);
			state = NORM;
			break;
		case ARGP_ADD_NM:
			name = arg;
			state = NORM;
			break;
		}
	}

	/* arg errors */
	if (file == NULL) {
		ERR("please provide filename");
		return -1;
	}


	/* inv manage */
	size_t len;
	struct item *items = inv_get(file, &len);
	if (items == NULL) {
		ERR("could not open file");
		return -1;
	}

	switch (op) {
	case ADD:
		if (name == NULL) {
			ERR("provide name for item");
			return -1;
		}

		struct item it = { .count = count };
		strcpy(it.name, name);
		if (url != NULL)
			strcpy(it.url, url);
		if (!inv_add(&items, &len, it)) {
			ERR("could not add item");
			return -1;
		}
		break;
	case LIST:
		inv_list(items, len);
		break;
	default:
		break;
	}

	if (!inv_set(file, items, len)) {
		ERR("could not write file");
		return -1;
	}
	free(items);

	/* TODO add git commit here */

	return 0;
}
