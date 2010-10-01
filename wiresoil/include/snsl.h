#ifndef SNSL_H
#define SNSL_H
#include <stdio.h>
#include "vdip.h"

#define MAX_NODE_NAME_LEN 3+1
#define FILE_NODES "NODES.TXT"

char** SNSL_ParseNodeNames(void);

#endif // SNSL_H
