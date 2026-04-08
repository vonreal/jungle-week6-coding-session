#include "executor.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const TableDef *find_table_def(const TableDef *tables, int table_count, const char *name) {
    int i;

    for (i = 0; i < table_count; i++) {
        if (strcmp(tables[i].name, name) == 0) {
            return &tables[i];
        }
    }

    return NULL;
}

static int print_error(ErrorCode code) {
    switch (code) {
        case ERR_TABLE_NOT_FOUND:
            fprintf(stderr, "ERROR: table not found\n");
            return -1;
        case ERR_COLUMN_MISMATCH:
            fprintf(stderr, "ERROR: column count does not match value count\n");
            return -1;
        case ERR_INVALID_QUERY:
            fprintf(stderr, "ERROR: invalid query\n");
            return -1;
        case ERR_FILE_OPEN:
            fprintf(stderr, "ERROR: file open failed\n");
            return -1;
        case ERR_NONE:
        default:
            return 0;
    }
}

static ErrorCode validate_insert_shape(const Command *cmd, const TableDef *table) {
    int i;

    if (cmd->insert_column_count != table->column_count) {
        return ERR_COLUMN_MISMATCH;
    }

    for (i = 0; i < table->column_count; i++) {
        if (strcmp(cmd->insert_columns[i], table->columns[i].name) != 0) {
            return ERR_INVALID_QUERY;
        }
    }

    return ERR_NONE;
}

static void print_result_set(const ResultSet *result) {
    int row;
    int col;

    if (result->row_count == 0) {
        return;
    }

    for (col = 0; col < result->header_count; col++) {
        if (col > 0) {
            putchar(',');
        }
        fputs(result->headers[col], stdout);
    }
    putchar('\n');

    for (row = 0; row < result->row_count; row++) {
        for (col = 0; col < result->header_count; col++) {
            if (col > 0) {
                putchar(',');
            }
            fputs(result->rows[row][col], stdout);
        }
        putchar('\n');
    }
}

int execute_command(const Command *cmd, StorageOps *ops, const TableDef *tables, int table_count) {
    if (cmd->type == CMD_INSERT) {
        const TableDef *table = find_table_def(tables, table_count, cmd->table_name);
        ErrorCode status;

        if (table == NULL) {
            return print_error(ERR_TABLE_NOT_FOUND);
        }

        status = validate_insert_shape(cmd, table);
        if (status != ERR_NONE) {
            return print_error(status);
        }

        status = (ErrorCode)ops->insert(ops->ctx, cmd->table_name, cmd->values, cmd->value_count);
        return print_error(status);
    }

    if (cmd->type == CMD_SELECT) {
        ResultSet *result = (ResultSet *)malloc(sizeof(*result));
        ErrorCode status;

        if (result == NULL) {
            return print_error(ERR_INVALID_QUERY);
        }

        memset(result, 0, sizeof(*result));
        status = (ErrorCode)ops->select_rows(ops->ctx,
                                             cmd->table_name,
                                             cmd->columns,
                                             cmd->column_count,
                                             cmd->is_select_all,
                                             result);
        if (status != ERR_NONE) {
            free(result);
            return print_error(status);
        }

        print_result_set(result);
        free(result);
        return 0;
    }

    return 0;
}
