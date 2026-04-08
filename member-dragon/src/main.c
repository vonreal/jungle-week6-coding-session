#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "executor.h"
#include "file_storage.h"
#include "parser.h"
#include "schema.h"

static int has_non_whitespace(const char *text) {
    while (*text != '\0') {
        if (!isspace((unsigned char)*text)) {
            return 1;
        }
        text++;
    }
    return 0;
}

static int read_next_statement(FILE *fp, char *buffer, size_t buffer_size) {
    int ch;
    int in_quote = 0;
    int overflow = 0;
    size_t len = 0;

    while ((ch = fgetc(fp)) != EOF) {
        if (!overflow) {
            if (len + 1 < buffer_size) {
                buffer[len++] = (char)ch;
            } else {
                overflow = 1;
            }
        }

        if (ch == '\'') {
            in_quote = !in_quote;
        }

        if (ch == ';' && !in_quote) {
            break;
        }
    }

    buffer[len] = '\0';

    if (ch == EOF && !has_non_whitespace(buffer)) {
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[]) {
    FILE *fp;
    TableDef tables[MAX_TABLES];
    int table_count;
    StorageOps *ops;
    int has_error = 0;
    char statement[8192];

    if (argc < 2) {
        fprintf(stderr, "ERROR: file open failed\n");
        return 1;
    }

    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: file open failed\n");
        return 1;
    }

    table_count = load_schemas(".", tables, MAX_TABLES);
    if (table_count < 0) {
        fclose(fp);
        fprintf(stderr, "ERROR: file open failed\n");
        return 1;
    }

    ops = file_storage_create();
    if (ops == NULL || ops->init == NULL || ops->insert == NULL ||
        ops->select_rows == NULL || ops->destroy == NULL ||
        ops->init(ops->ctx, tables, table_count) != ERR_NONE) {
        if (ops != NULL) {
            if (ops->destroy != NULL && ops->ctx != NULL) {
                ops->destroy(ops->ctx);
            }
            free(ops);
        }
        fclose(fp);
        fprintf(stderr, "ERROR: file open failed\n");
        return 1;
    }

    while (read_next_statement(fp, statement, sizeof(statement))) {
        Command cmd;

        if (parse_sql(statement, &cmd) != 0) {
            fprintf(stderr, "ERROR: invalid query\n");
            has_error = 1;
            continue;
        }

        if (cmd.type == CMD_NONE) {
            continue;
        }

        if (execute_command(&cmd, ops, tables, table_count) != 0) {
            has_error = 1;
        }
    }

    fclose(fp);
    ops->destroy(ops->ctx);
    free(ops);

    return has_error ? 1 : 0;
}
