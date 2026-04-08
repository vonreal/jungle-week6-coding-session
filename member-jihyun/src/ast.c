#include "include/ast.h"

#include <stdlib.h>

/* 문자열 포인터 배열을 순회하며 각 원소와 배열 자체를 해제한다. */
static void free_string_array(char **items, size_t count)
{
    size_t index;

    if (items == NULL) {
        return;
    }

    for (index = 0U; index < count; index++) {
        free(items[index]);
    }

    free(items);
}

/* AST 문장 타입에 맞춰 내부 동적 메모리를 정리한다. */
void statement_destroy(Statement *statement)
{
    if (statement == NULL) {
        return;
    }

    if (statement->type == STATEMENT_INSERT) {
        free(statement->as.insert_stmt.table_name);
        free_string_array(statement->as.insert_stmt.columns, statement->as.insert_stmt.column_count);
        free_string_array(statement->as.insert_stmt.values, statement->as.insert_stmt.value_count);
    } else if (statement->type == STATEMENT_SELECT) {
        free(statement->as.select_stmt.table_name);
        free_string_array(statement->as.select_stmt.columns, statement->as.select_stmt.column_count);
    }
}
