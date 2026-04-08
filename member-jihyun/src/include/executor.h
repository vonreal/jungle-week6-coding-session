#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stdbool.h>
#include <stdio.h>

#include "ast.h"
#include "storage.h"

typedef struct ExecutionContext {
    char schema_dir[MAX_PATH_LENGTH];
    StorageEngine storage;
} ExecutionContext;

SqlStatus execution_context_init(ExecutionContext *context, const char *schema_dir, const char *data_dir);
SqlStatus execution_context_flush(ExecutionContext *context);
void execution_context_destroy(ExecutionContext *context);
SqlStatus execute_statement(
    ExecutionContext *context,
    const Statement *statement,
    FILE *output_stream,
    FILE *error_stream,
    bool *did_error
);

#endif
