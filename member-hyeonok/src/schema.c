#include "schema.h"

#include "util.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void initialize_schema(Schema *schema)
{
    schema->table_name = NULL;
    schema->columns = NULL;
    schema->column_count = 0;
}

static void skip_json_spaces(const char **cursor)
{
    while (**cursor != '\0' && isspace((unsigned char)**cursor)) {
        (*cursor)++;
    }
}

static int expect_char(const char **cursor, char expected)
{
    skip_json_spaces(cursor);
    if (**cursor != expected) {
        return 0;
    }

    (*cursor)++;
    return 1;
}

static int parse_json_string(const char **cursor, char **out_text)
{
    const char *start;
    char *text;

    skip_json_spaces(cursor);
    if (**cursor != '"') {
        return 0;
    }
    (*cursor)++;
    start = *cursor;

    /*
     * 과제 스키마는 단순 문자열만 사용하므로,
     * 이번 단계에서는 이스케이프 시퀀스를 지원하지 않는 제한적 파서를 둔다.
     */
    while (**cursor != '\0' && **cursor != '"') {
        if (**cursor == '\\') {
            return 0;
        }
        (*cursor)++;
    }

    if (**cursor != '"') {
        return 0;
    }

    text = (char *)malloc((size_t)(*cursor - start) + 1);
    if (text == NULL) {
        return 0;
    }

    memcpy(text, start, (size_t)(*cursor - start));
    text[*cursor - start] = '\0';
    (*cursor)++;
    *out_text = text;
    return 1;
}

static int parse_column_type(const char *type_name, ColumnType *type)
{
    if (strcmp(type_name, "string") == 0) {
        *type = COLUMN_TYPE_STRING;
        return 1;
    }

    if (strcmp(type_name, "int") == 0) {
        *type = COLUMN_TYPE_INT;
        return 1;
    }

    return 0;
}

static int append_column_schema(Schema *schema, const ColumnSchema *column)
{
    ColumnSchema *new_columns;

    new_columns = (ColumnSchema *)realloc(
        schema->columns,
        sizeof(ColumnSchema) * (schema->column_count + 1)
    );
    if (new_columns == NULL) {
        return 0;
    }

    schema->columns = new_columns;
    schema->columns[schema->column_count] = *column;
    schema->column_count++;
    return 1;
}

static int parse_column_object(const char **cursor, ColumnSchema *column)
{
    char *key;
    char *value;
    int has_name;
    int has_type;

    column->name = NULL;
    column->type = COLUMN_TYPE_STRING;
    has_name = 0;
    has_type = 0;

    if (!expect_char(cursor, '{')) {
        return 0;
    }

    while (1) {
        if (!parse_json_string(cursor, &key)) {
            free(column->name);
            column->name = NULL;
            return 0;
        }

        if (!expect_char(cursor, ':')) {
            free(key);
            free(column->name);
            column->name = NULL;
            return 0;
        }

        if (!parse_json_string(cursor, &value)) {
            free(key);
            free(column->name);
            column->name = NULL;
            return 0;
        }

        if (strcmp(key, "name") == 0) {
            free(column->name);
            column->name = value;
            has_name = 1;
        } else if (strcmp(key, "type") == 0) {
            if (!parse_column_type(value, &column->type)) {
                free(key);
                free(value);
                free(column->name);
                column->name = NULL;
                return 0;
            }
            free(value);
            has_type = 1;
        } else {
            free(value);
        }

        free(key);

        skip_json_spaces(cursor);
        if (**cursor == ',') {
            (*cursor)++;
            continue;
        }

        break;
    }

    if (!expect_char(cursor, '}')) {
        free(column->name);
        column->name = NULL;
        return 0;
    }

    if (!has_name || !has_type) {
        free(column->name);
        column->name = NULL;
        return 0;
    }

    return 1;
}

static int parse_columns_array(const char **cursor, Schema *schema)
{
    ColumnSchema column;

    if (!expect_char(cursor, '[')) {
        return 0;
    }

    skip_json_spaces(cursor);
    if (**cursor == ']') {
        (*cursor)++;
        return 1;
    }

    while (1) {
        if (!parse_column_object(cursor, &column)) {
            return 0;
        }

        if (!append_column_schema(schema, &column)) {
            free(column.name);
            return 0;
        }

        skip_json_spaces(cursor);
        if (**cursor == ',') {
            (*cursor)++;
            continue;
        }

        break;
    }

    if (!expect_char(cursor, ']')) {
        return 0;
    }

    return 1;
}

static int parse_schema_json(const char *json_text, Schema *schema)
{
    const char *cursor;
    char *key;
    char *value;
    int has_table;
    int has_columns;

    cursor = json_text;
    has_table = 0;
    has_columns = 0;

    if (!expect_char(&cursor, '{')) {
        return 0;
    }

    while (1) {
        skip_json_spaces(&cursor);
        if (*cursor == '}') {
            cursor++;
            break;
        }

        if (!parse_json_string(&cursor, &key)) {
            return 0;
        }

        if (!expect_char(&cursor, ':')) {
            free(key);
            return 0;
        }

        if (strcmp(key, "table") == 0) {
            if (!parse_json_string(&cursor, &value)) {
                free(key);
                return 0;
            }
            schema->table_name = value;
            has_table = 1;
        } else if (strcmp(key, "columns") == 0) {
            if (!parse_columns_array(&cursor, schema)) {
                free(key);
                return 0;
            }
            has_columns = 1;
        } else {
            free(key);
            return 0;
        }

        free(key);

        skip_json_spaces(&cursor);
        if (*cursor == ',') {
            cursor++;
            continue;
        }

        if (*cursor == '}') {
            cursor++;
            break;
        }

        return 0;
    }

    skip_json_spaces(&cursor);
    if (*cursor != '\0') {
        return 0;
    }

    return has_table && has_columns && schema->column_count > 0;
}

SchemaLoadResult load_schema_for_table(const char *table_name, Schema *schema)
{
    char path_buffer[512];
    FILE *file;
    char *json_text;
    int parse_ok;

    if (table_name == NULL || schema == NULL) {
        return SCHEMA_LOAD_FILE_ERROR;
    }

    initialize_schema(schema);

    /*
     * 테스트 러너는 실행 전에 .schema 파일을 현재 작업 디렉토리로 복사하므로,
     * 공통 폴더 경로를 하드코딩하지 않고 파일명만 조합해서 찾는다.
     */
    if (snprintf(path_buffer, sizeof(path_buffer), "%s.schema", table_name) >=
        (int)sizeof(path_buffer)) {
        return SCHEMA_LOAD_FILE_ERROR;
    }

    file = fopen(path_buffer, "rb");
    if (file == NULL) {
        if (errno == ENOENT) {
            return SCHEMA_LOAD_TABLE_NOT_FOUND;
        }
        return SCHEMA_LOAD_FILE_ERROR;
    }
    fclose(file);

    json_text = read_entire_file(path_buffer);
    if (json_text == NULL) {
        return SCHEMA_LOAD_FILE_ERROR;
    }

    parse_ok = parse_schema_json(json_text, schema);
    free(json_text);

    if (!parse_ok) {
        free_schema(schema);
        return SCHEMA_LOAD_FILE_ERROR;
    }

    return SCHEMA_LOAD_SUCCESS;
}

void free_schema(Schema *schema)
{
    size_t index;

    if (schema == NULL) {
        return;
    }

    free(schema->table_name);
    schema->table_name = NULL;

    if (schema->columns != NULL) {
        for (index = 0; index < schema->column_count; index++) {
            free(schema->columns[index].name);
        }
        free(schema->columns);
    }

    schema->columns = NULL;
    schema->column_count = 0;
}
