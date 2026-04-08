#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stddef.h>

typedef enum StatementType {
    STATEMENT_INSERT = 0,
    STATEMENT_SELECT
} StatementType;

typedef struct InsertStatement {
    char *table_name;
    size_t column_count;
    char **columns;
    size_t value_count;
    char **values;
} InsertStatement;

typedef struct SelectStatement {
    char *table_name;
    bool select_all;
    size_t column_count;
    char **columns;
} SelectStatement;

typedef struct Statement {
    StatementType type;
    union {
        InsertStatement insert_stmt;
        SelectStatement select_stmt;
    } as;
} Statement;

void statement_destroy(Statement *statement);

#endif
