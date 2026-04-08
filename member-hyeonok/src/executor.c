#include "executor.h"

#include "schema.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int find_schema_column_index(const Schema *schema, const char *column_name)
{
    size_t index;

    for (index = 0; index < schema->column_count; index++) {
        if (strcmp(schema->columns[index].name, column_name) == 0) {
            return (int)index;
        }
    }

    return -1;
}

static int write_escaped_text(FILE *file, const char *text)
{
    const char *cursor;

    /*
     * 데이터 파일은 탭 구분 텍스트 포맷을 사용한다.
     * 값 내부의 역슬래시, 탭, 개행은 이스케이프해서 다음 SELECT 단계가
     * 안정적으로 역직렬화할 수 있도록 저장한다.
     */
    for (cursor = text; *cursor != '\0'; cursor++) {
        if (*cursor == '\\') {
            if (fputs("\\\\", file) == EOF) {
                return 0;
            }
        } else if (*cursor == '\t') {
            if (fputs("\\t", file) == EOF) {
                return 0;
            }
        } else if (*cursor == '\n') {
            if (fputs("\\n", file) == EOF) {
                return 0;
            }
        } else {
            if (fputc(*cursor, file) == EOF) {
                return 0;
            }
        }
    }

    return 1;
}

static ExecuteResult append_insert_row(
    const InsertStatement *insert,
    const Schema *schema
)
{
    char path_buffer[512];
    const SqlValue **ordered_values;
    size_t insert_index;
    size_t schema_index;
    int found_index;
    FILE *file;
    ExecuteResult result;

    if (snprintf(path_buffer, sizeof(path_buffer), "%s.data", insert->table_name) >=
        (int)sizeof(path_buffer)) {
        return EXECUTE_FILE_ERROR;
    }

    ordered_values = (const SqlValue **)calloc(schema->column_count, sizeof(SqlValue *));
    if (ordered_values == NULL) {
        return EXECUTE_FILE_ERROR;
    }

    /*
     * INSERT 컬럼 순서가 스키마와 달라도 저장 파일은 항상 스키마 순서로 맞춘다.
     * 이렇게 두면 이후 SELECT 구현에서 헤더와 데이터 행을 같은 기준으로 읽을 수 있다.
     */
    for (insert_index = 0; insert_index < insert->column_count; insert_index++) {
        found_index = find_schema_column_index(schema, insert->columns[insert_index]);
        if (found_index < 0) {
            free(ordered_values);
            return EXECUTE_INVALID_QUERY;
        }

        schema_index = (size_t)found_index;
        if (ordered_values[schema_index] != NULL) {
            free(ordered_values);
            return EXECUTE_INVALID_QUERY;
        }

        ordered_values[schema_index] = &insert->values[insert_index];
    }

    for (schema_index = 0; schema_index < schema->column_count; schema_index++) {
        if (ordered_values[schema_index] == NULL) {
            free(ordered_values);
            return EXECUTE_INVALID_QUERY;
        }
    }

    file = fopen(path_buffer, "a");
    if (file == NULL) {
        free(ordered_values);
        return EXECUTE_FILE_ERROR;
    }

    result = EXECUTE_SUCCESS;
    for (schema_index = 0; schema_index < schema->column_count; schema_index++) {
        if (schema_index > 0 && fputc('\t', file) == EOF) {
            result = EXECUTE_FILE_ERROR;
            break;
        }

        if (!write_escaped_text(file, ordered_values[schema_index]->text)) {
            result = EXECUTE_FILE_ERROR;
            break;
        }
    }

    if (result == EXECUTE_SUCCESS && fputc('\n', file) == EOF) {
        result = EXECUTE_FILE_ERROR;
    }

    if (fclose(file) != 0 && result == EXECUTE_SUCCESS) {
        result = EXECUTE_FILE_ERROR;
    }

    free(ordered_values);
    return result;
}

static ExecuteResult execute_insert_statement(const InsertStatement *insert)
{
    Schema schema;
    SchemaLoadResult schema_result;
    ExecuteResult result;

    schema_result = load_schema_for_table(insert->table_name, &schema);
    if (schema_result == SCHEMA_LOAD_TABLE_NOT_FOUND) {
        return EXECUTE_TABLE_NOT_FOUND;
    }

    if (schema_result != SCHEMA_LOAD_SUCCESS) {
        return EXECUTE_FILE_ERROR;
    }

    /*
     * 과제 명세상 가장 먼저 보장해야 하는 검사는 컬럼 수와 값 수의 일치 여부다.
     * 이 조건이 맞지 않으면 데이터 파일에는 아무것도 기록하지 않는다.
     */
    if (insert->column_count != insert->value_count) {
        free_schema(&schema);
        return EXECUTE_COLUMN_COUNT_MISMATCH;
    }

    if (insert->column_count != schema.column_count) {
        free_schema(&schema);
        return EXECUTE_COLUMN_COUNT_MISMATCH;
    }

    result = append_insert_row(insert, &schema);
    free_schema(&schema);
    return result;
}

ExecuteResult execute_sql_statement(const SqlStatement *statement)
{
    if (statement == NULL) {
        return EXECUTE_FILE_ERROR;
    }

    if (statement->type == SQL_STATEMENT_INSERT) {
        return execute_insert_statement(&statement->insert);
    }

    /*
     * SELECT 조회는 다음 단계에서 구현한다.
     * 현재 단계에서는 INSERT 저장만 실제 동작 대상으로 본다.
     */
    if (statement->type == SQL_STATEMENT_SELECT) {
        return EXECUTE_SUCCESS;
    }

    return EXECUTE_INVALID_QUERY;
}
