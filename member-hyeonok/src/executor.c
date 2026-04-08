#include "executor.h"

#include "schema.h"
#include "util.h"

#include <errno.h>
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

static int *build_select_column_indexes(
    const SelectStatement *select,
    const Schema *schema,
    size_t *selected_count,
    ExecuteResult *result
)
{
    int *indexes;
    size_t index;
    int found_index;

    if (select->select_all) {
        indexes = (int *)malloc(sizeof(int) * schema->column_count);
        if (indexes == NULL) {
            *result = EXECUTE_FILE_ERROR;
            return NULL;
        }

        for (index = 0; index < schema->column_count; index++) {
            indexes[index] = (int)index;
        }

        *selected_count = schema->column_count;
        *result = EXECUTE_SUCCESS;
        return indexes;
    }

    indexes = (int *)malloc(sizeof(int) * select->column_count);
    if (indexes == NULL) {
        *result = EXECUTE_FILE_ERROR;
        return NULL;
    }

    for (index = 0; index < select->column_count; index++) {
        found_index = find_schema_column_index(schema, select->columns[index]);
        if (found_index < 0) {
            free(indexes);
            *result = EXECUTE_INVALID_QUERY;
            return NULL;
        }
        indexes[index] = found_index;
    }

    *selected_count = select->column_count;
    *result = EXECUTE_SUCCESS;
    return indexes;
}

static int parse_escaped_field(const char **cursor, char **out_text)
{
    size_t capacity;
    size_t length;
    char *buffer;
    char current;

    capacity = 16;
    length = 0;
    buffer = (char *)malloc(capacity);
    if (buffer == NULL) {
        return 0;
    }

    while (**cursor != '\0' && **cursor != '\t' && **cursor != '\n') {
        current = **cursor;
        (*cursor)++;

        if (current == '\\') {
            if (**cursor == '\0') {
                free(buffer);
                return 0;
            }

            current = **cursor;
            (*cursor)++;

            if (current == 't') {
                current = '\t';
            } else if (current == 'n') {
                current = '\n';
            } else if (current == '\\') {
                current = '\\';
            } else {
                free(buffer);
                return 0;
            }
        }

        if (length + 1 >= capacity) {
            char *new_buffer;

            capacity *= 2;
            new_buffer = (char *)realloc(buffer, capacity);
            if (new_buffer == NULL) {
                free(buffer);
                return 0;
            }
            buffer = new_buffer;
        }

        buffer[length] = current;
        length++;
    }

    buffer[length] = '\0';
    *out_text = buffer;
    return 1;
}

static int parse_data_row(
    const char *line_start,
    size_t expected_count,
    char ***out_fields
)
{
    const char *cursor;
    char **fields;
    size_t index;

    cursor = line_start;
    fields = (char **)calloc(expected_count, sizeof(char *));
    if (fields == NULL) {
        return 0;
    }

    for (index = 0; index < expected_count; index++) {
        if (!parse_escaped_field(&cursor, &fields[index])) {
            free_string_array(fields, expected_count);
            return 0;
        }

        if (index + 1 < expected_count) {
            if (*cursor != '\t') {
                free_string_array(fields, expected_count);
                return 0;
            }
            cursor++;
        }
    }

    if (*cursor != '\0' && *cursor != '\n') {
        free_string_array(fields, expected_count);
        return 0;
    }

    *out_fields = fields;
    return 1;
}

static int print_selected_header(
    const SelectStatement *select,
    const Schema *schema,
    const int *indexes,
    size_t selected_count
)
{
    size_t index;

    for (index = 0; index < selected_count; index++) {
        if (index > 0 && fputc(',', stdout) == EOF) {
            return 0;
        }

        if (select->select_all) {
            if (fputs(schema->columns[indexes[index]].name, stdout) == EOF) {
                return 0;
            }
        } else {
            if (fputs(select->columns[index], stdout) == EOF) {
                return 0;
            }
        }
    }

    return fputc('\n', stdout) != EOF;
}

static int print_selected_row(
    char **fields,
    const int *indexes,
    size_t selected_count
)
{
    size_t index;

    for (index = 0; index < selected_count; index++) {
        if (index > 0 && fputc(',', stdout) == EOF) {
            return 0;
        }

        if (fputs(fields[indexes[index]], stdout) == EOF) {
            return 0;
        }
    }

    return fputc('\n', stdout) != EOF;
}

static ExecuteResult execute_select_statement(const SelectStatement *select)
{
    Schema schema;
    SchemaLoadResult schema_result;
    char path_buffer[512];
    FILE *file;
    char *data_text;
    char *cursor;
    int *selected_indexes;
    size_t selected_count;
    int printed_header;
    ExecuteResult result;

    schema_result = load_schema_for_table(select->table_name, &schema);
    if (schema_result == SCHEMA_LOAD_TABLE_NOT_FOUND) {
        return EXECUTE_TABLE_NOT_FOUND;
    }

    if (schema_result != SCHEMA_LOAD_SUCCESS) {
        return EXECUTE_FILE_ERROR;
    }

    selected_indexes = build_select_column_indexes(
        select,
        &schema,
        &selected_count,
        &result
    );
    if (selected_indexes == NULL) {
        free_schema(&schema);
        return result;
    }

    if (snprintf(path_buffer, sizeof(path_buffer), "%s.data", select->table_name) >=
        (int)sizeof(path_buffer)) {
        free(selected_indexes);
        free_schema(&schema);
        return EXECUTE_FILE_ERROR;
    }

    file = fopen(path_buffer, "rb");
    if (file == NULL) {
        free(selected_indexes);
        free_schema(&schema);
        if (errno == ENOENT) {
            return EXECUTE_SUCCESS;
        }
        return EXECUTE_FILE_ERROR;
    }
    fclose(file);

    data_text = read_entire_file(path_buffer);
    if (data_text == NULL) {
        free(selected_indexes);
        free_schema(&schema);
        return EXECUTE_FILE_ERROR;
    }

    printed_header = 0;
    result = EXECUTE_SUCCESS;
    cursor = data_text;

    while (*cursor != '\0') {
        char *line_start;
        char *line_end;
        char saved_char;
        char **fields;

        line_start = cursor;
        line_end = strchr(cursor, '\n');
        if (line_end == NULL) {
            line_end = cursor + strlen(cursor);
            saved_char = *line_end;
        } else {
            saved_char = *line_end;
            *line_end = '\0';
        }

        /*
         * 빈 데이터 파일이거나 마지막 빈 줄은 출력 없이 건너뛴다.
         * 실제 row가 하나라도 있을 때만 헤더를 먼저 출력한다.
         */
        if (*line_start != '\0') {
            if (!parse_data_row(line_start, schema.column_count, &fields)) {
                result = EXECUTE_FILE_ERROR;
            } else {
                if (!printed_header) {
                    if (!print_selected_header(select, &schema, selected_indexes, selected_count)) {
                        free_string_array(fields, schema.column_count);
                        result = EXECUTE_FILE_ERROR;
                    } else {
                        printed_header = 1;
                    }
                }

                if (result == EXECUTE_SUCCESS &&
                    !print_selected_row(fields, selected_indexes, selected_count)) {
                    result = EXECUTE_FILE_ERROR;
                }

                free_string_array(fields, schema.column_count);
            }
        }

        if (saved_char == '\n') {
            *line_end = '\n';
            cursor = line_end + 1;
        } else {
            cursor = line_end;
        }

        if (result != EXECUTE_SUCCESS) {
            break;
        }
    }

    free(data_text);
    free(selected_indexes);
    free_schema(&schema);
    return result;
}

ExecuteResult execute_query(const SqlStatement *statement)
{
    if (statement == NULL) {
        return EXECUTE_FILE_ERROR;
    }

    if (statement->type == SQL_STATEMENT_INSERT) {
        return execute_insert_statement(&statement->insert);
    }

    if (statement->type == SQL_STATEMENT_SELECT) {
        return execute_select_statement(&statement->select);
    }

    return EXECUTE_INVALID_QUERY;
}

ExecuteResult execute_sql_statement(const SqlStatement *statement)
{
    return execute_query(statement);
}
