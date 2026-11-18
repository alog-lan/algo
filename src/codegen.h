#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <stdio.h>

void codegen_generate(Program *prog, FILE *out);

#endif // CODEGEN_H
