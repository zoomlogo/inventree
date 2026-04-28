/* Suckless inventory manager. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ERR(msg) fprintf(stderr, msg "\n")

enum operation { NONE, ADD, REMOVE, UPDATE, FIND, LIST };

#define SZ_NAME   64
#define SZ_DESC   256
#define SZ_URL    256
#define MAX_NAMES 16
struct item {
	char name[SZ_NAME];
	char desc[SZ_DESC];
	int count;
	int len;
	char names[MAX_NAMES][SZ_NAME];
	int counts[MAX_NAMES];
	char url[SZ_URL];
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
			i->count += it.count;
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

bool
inv_update(struct item *items, size_t len, char *name,
           char *pname, int count, bool remove, char *desc, char *url)
{
	struct item *it = NULL;
	for (struct item *i = items; i < items + len; ++i) {
		if (strcmp(i->name, name) == 0) {
			it = i;
			break;
		}
	}
	if (it == NULL)
		return false;

	if (pname == NULL && count != 1)
		it->count = count;
	if (desc != NULL)
		strcpy(it->desc, desc);
	if (url != NULL)
		strcpy(it->url, url);

	if (pname != NULL) {
		if (!remove) {
			for (int i = 0; i < MAX_NAMES; ++i) {
				if (strcmp(pname, it->names[i]) == 0) {
					it->counts[i] += count;
					return true;
				}
			}

			strcpy(it->names[it->len], pname);
			it->counts[it->len] = count;
			it->len++;
		} else for (int i = 0; i < MAX_NAMES; ++i) {
			if (strcmp(pname, it->names[i]) == 0) {
				if (i != it->len - 1) {
					strcpy(it->names[i], it->names[it->len - 1]);
					memcpy(&it->counts[i],
					       &it->counts[it->len - 1], sizeof(int));
				}
				it->len--;
				return true;
			}
		}
	}
	return true;
}

void
inv_list(struct item *items, size_t len)
{
	printf("NAME\tDESC\tCOUNT\tLINK\n");
	for (size_t i = 0; i < len; ++i) {
		struct item it = items[i];
		printf("%s\t%s\t%d\t%s\n", it.name, it.desc, it.count, it.url);
	}
}

void
inv_find(struct item *items, size_t len,
         bool search_item, bool search_person, char *name, char *pname)
{
	bool *mask_item = calloc(len, sizeof(bool));
	bool *mask_person = calloc(len, sizeof(bool));

	if (search_item) {
		for (size_t i = 0; i < len; ++i) {
			if (strcmp(name, items[i].name) == 0)
				mask_item[i] = true;
		}
	} else for (size_t i = 0; i < len; ++i) {
		mask_item[i] = true;
	}

	if (search_person) {
		for (size_t i = 0; i < len; ++i) {
			for (size_t j = 0; j < items[i].len; ++j) {
				if (strcmp(pname, items[i].names[j]) == 0)
					mask_person[i] = true;
			}
		}
	} else for (size_t i = 0; i < len; ++i) {
		mask_person[i] = true;
	}

	printf("NAME\tDESC\tCOUNT\tWHO HAS\tLINK\n");
	for (size_t i = 0; i < len; ++i) {
		if (mask_item[i] && mask_person[i]) {
			struct item it = items[i];
			printf("%s\t%s\t%d\t", it.name, it.desc, it.count);
			for (size_t j = 0; j < it.len; ++j) {
				printf("%s:%d", it.names[j], it.counts[j]);
				if (j < it.len - 1)
					printf(",");
			}
			printf("\t%s\n", it.url);
		}
	}

	free(mask_person);
	free(mask_item);
}

enum argstate { NORM, ARGP_FILE, ARGP_COUNT, ARGP_NM, ARGP_URL, ARGP_DESC, ARGP_PNM };
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
	char *desc = NULL;
	bool search_item = false;
	bool search_person = false;
	bool remove = false;
	char *pname = NULL;

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
				state = ARGP_NM;
			} else if (ARG_IS("-R", "-remove")) {
				op = REMOVE;
				state = ARGP_NM;
			} else if (ARG_IS("-U", "-update")) {
				op = UPDATE;
				state = ARGP_NM;
			} else if (ARG_IS("-F", "-find"))
				op = FIND;
			else if (ARG_IS("-L", "-list"))
				op = LIST;
			else if (ARG_IS("-I", "-inventory"))
				state = ARGP_FILE;
			else if (ARG_IS("-n", "-count"))
				state = ARGP_COUNT;
			else if (ARG_IS("-l", "-link"))
				state = ARGP_URL;
			else if (ARG_IS("-d", "-desc"))
				state = ARGP_DESC;
			else if (ARG_IS("-i", "-search-item")) {
				state = ARGP_NM;
				search_item = true;
			} else if (ARG_IS("-w", "-search-person")) {
				state = ARGP_PNM;
				search_person = true;
			} else if (ARG_IS("-p", "-person"))
				state = ARGP_PNM;
			else if (ARG_IS("-r", "-remove-person")) {
				state = ARGP_PNM;
				remove = true;
			}
			break;
		case ARGP_FILE:
			file = arg;
			state = NORM;
			break;
		case ARGP_COUNT:
			count = atoi(arg);
			state = NORM;
			break;
		case ARGP_NM:
			name = arg;
			state = NORM;
			break;
		case ARGP_DESC:
			desc = arg;
			state = NORM;
			break;
		case ARGP_URL:
			url = arg;
			state = NORM;
			break;
		case ARGP_PNM:
			pname = arg;
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
		if (desc != NULL)
			strcpy(it.desc, desc);
		if (!inv_add(&items, &len, it)) {
			ERR("could not add item");
			return -1;
		}
		break;

	case REMOVE:
		if (name == NULL) {
			ERR("provide name for item");
			return -1;
		}
		for (struct item *i = items; i < items + len; ++i) {
			if (strcmp(i->name, name) == 0) {
				/* copy last index here and decrease len */
				if (len > 0 && i != items + len - 1)
					memcpy(i, &items[len - 1], sizeof(struct item));
				len--;
				break;
			}
		}
		break;

	case UPDATE:
		if (name == NULL) {
			ERR("provide name for item");
			return -1;
		}
		if (!inv_update(items, len, name, pname, count, remove, desc, url)) {
			ERR("failed to update inv");
			return -1;
		}
		break;

	case FIND:  /* search for person OR item OR (person AND item) if given */
		if (search_item && name == NULL) {
			ERR("provide name for item");
			return -1;
		}
		if (search_person && pname == NULL) {
			ERR("provide name for person");
			return -1;
		}
		inv_find(items, len, search_item, search_person, name, pname);
		break;

	case LIST:
		inv_list(items, len);
		break;
	}

	if (!inv_set(file, items, len)) {
		ERR("could not write file");
		return -1;
	}
	free(items);

	return 0;
}
