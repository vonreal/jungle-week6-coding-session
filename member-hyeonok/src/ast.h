#ifndef AST_H
#define AST_H

#include <stddef.h>

typedef enum SqlStatementType {
    SQL_STATEMENT_UNCLASSIFIED = 0,
    SQL_STATEMENT_INSERT = 1
} SqlStatementType;

typedef enum SqlValueType {
    SQL_VALUE_STRING = 0,
    SQL_VALUE_INT = 1
} SqlValueType;

typedef struct SqlValue {
    SqlValueType type;
    char *text;
    int int_value;
} SqlValue;

typedef struct InsertStatement {
    char *table_name;
    char **columns;
    size_t column_count;
    SqlValue *values;
    size_t value_count;
} InsertStatement;

typedef struct SqlStatement {
    SqlStatementType type;
    char *raw_sql;
    InsertStatement insert;
} SqlStatement;

#endif
