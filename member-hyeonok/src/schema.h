#ifndef SCHEMA_H
#define SCHEMA_H

#include <stddef.h>

typedef enum ColumnType {
    COLUMN_TYPE_STRING = 0,
    COLUMN_TYPE_INT = 1
} ColumnType;

typedef struct ColumnSchema {
    char *name;
    ColumnType type;
} ColumnSchema;

typedef struct Schema {
    char *table_name;
    ColumnSchema *columns;
    size_t column_count;
} Schema;

typedef enum SchemaLoadResult {
    SCHEMA_LOAD_SUCCESS = 0,
    SCHEMA_LOAD_TABLE_NOT_FOUND = 1,
    SCHEMA_LOAD_FILE_ERROR = 2
} SchemaLoadResult;

/*
 * 현재 작업 디렉토리 기준으로 <table>.schema 파일을 읽어 스키마를 로딩한다.
 * 과제에서 사용하는 제한적 JSON 형식만 지원한다.
 */
SchemaLoadResult load_schema_for_table(const char *table_name, Schema *schema);

/*
 * 스키마 구조체가 보유한 동적 메모리를 모두 해제한다.
 */
void free_schema(Schema *schema);

#endif
