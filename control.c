#include "control.h"

#include <stdlib.h>
#include <string.h>

void newCard(struct Card *c, const char *name, const char *path)
{
	c->name = strdup(name);
	c->path = strdup(path);
}

void deleteCard(struct Card *c)
{
	free((void *)c->name);
	free((void *)c->path);
}
