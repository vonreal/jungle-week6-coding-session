#include "include/executor.h"

#include <stdlib.h>
#include <string.h>

#include "include/schema.h"
#include "include/status.h"
#include "include/utils.h"

typedef struct SelectPrinterContext {
    FILE *output_stream;
    const TableSchema *schema;
    size_t column_count;
    int *column_indexes;
    bool header_written;
} SelectPrinterContext;

/* 상태 코드가 실패일 때 표준 에러 형식으로 출력한다. */
static void write_error(FILE *error_stream, SqlStatus status)
{
    if (status == SQL_STATUS_OK) {
        return;
    }

    fprintf(error_stream, "ERROR: %s\n", sql_status_message(status));
}

/* 실행 컨텍스트를 초기화하고 스토리지 엔진을 준비한다. */
SqlStatus execution_context_init(ExecutionContext *context, const char *schema_dir, const char *data_dir)
{
    memset(context, 0, sizeof(*context));
    strncpy(context->schema_dir, schema_dir, MAX_PATH_LENGTH - 1U);
    context->schema_dir[MAX_PATH_LENGTH - 1U] = '\0';
    return storage_engine_init(&context->storage, data_dir);
}

/* 캐시에 남은 변경사항을 파일로 반영한다. */
SqlStatus execution_context_flush(ExecutionContext *context)
{
    return storage_flush(&context->storage);
}

/* 실행 컨텍스트가 보유한 리소스를 해제한다. */
void execution_context_destroy(ExecutionContext *context)
{
    if (context == NULL) {
        return;
    }

    storage_engine_destroy(&context->storage);
}

/* INSERT용 정렬된 값 배열을 전부 해제한다. */
static void free_ordered_values(char **values, size_t count)
{
    size_t index;

    if (values == NULL) {
        return;
    }

    for (index = 0U; index < count; index++) {
        free(values[index]);
    }
    free(values);
}

/* INSERT 컬럼 순서를 스키마 순서로 재배치한 값 배열을 만든다. */
static SqlStatus build_insert_values(const TableSchema *schema, const InsertStatement *insert_stmt, char ***ordered_values)
{
    char **values;
    bool *assigned;
    size_t index;

    if (insert_stmt->column_count != insert_stmt->value_count || insert_stmt->column_count != schema->column_count) {
        return SQL_STATUS_COLUMN_VALUE_COUNT_MISMATCH;
    }

    values = (char **)calloc(schema->column_count, sizeof(char *));
    assigned = (bool *)calloc(schema->column_count, sizeof(bool));
    if (values == NULL || assigned == NULL) {
        free(values);
        free(assigned);
        return SQL_STATUS_INTERNAL_ERROR;
    }

    for (index = 0U; index < insert_stmt->column_count; index++) {
        int schema_index;

        schema_index = schema_find_column(schema, insert_stmt->columns[index]);
        if (schema_index < 0 || assigned[schema_index]) {
            free(assigned);
            free_ordered_values(values, schema->column_count);
            return SQL_STATUS_INVALID_QUERY;
        }

        values[schema_index] = util_strdup(insert_stmt->values[index]);
        if (values[schema_index] == NULL) {
            free(assigned);
            free_ordered_values(values, schema->column_count);
            return SQL_STATUS_INTERNAL_ERROR;
        }

        assigned[schema_index] = true;
    }

    free(assigned);
    *ordered_values = values;
    return SQL_STATUS_OK;
}

/* SELECT 대상 컬럼을 스키마 인덱스 목록으로 변환한다. */
static SqlStatus build_select_columns(const TableSchema *schema, const SelectStatement *select_stmt, int **out_indexes, size_t *out_count)
{
    size_t index;
    int *indexes;

    if (select_stmt->select_all) {
        indexes = (int *)calloc(schema->column_count, sizeof(int));
        if (indexes == NULL) {
            return SQL_STATUS_INTERNAL_ERROR;
        }

        for (index = 0U; index < schema->column_count; index++) {
            indexes[index] = (int)index;
        }

        *out_indexes = indexes;
        *out_count = schema->column_count;
        return SQL_STATUS_OK;
    }

    indexes = (int *)calloc(select_stmt->column_count, sizeof(int));
    if (indexes == NULL) {
        return SQL_STATUS_INTERNAL_ERROR;
    }

    for (index = 0U; index < select_stmt->column_count; index++) {
        indexes[index] = schema_find_column(schema, select_stmt->columns[index]);
        if (indexes[index] < 0) {
            free(indexes);
            return SQL_STATUS_INVALID_QUERY;
        }
    }

    *out_indexes = indexes;
    *out_count = select_stmt->column_count;
    return SQL_STATUS_OK;
}

/* 조회 결과의 헤더와 행 데이터를 CSV 형식으로 출력한다. */
static bool print_selected_row(const RowData *row, void *context)
{
    SelectPrinterContext *printer;
    size_t index;

    printer = (SelectPrinterContext *)context;

    if (!printer->header_written) {
        for (index = 0U; index < printer->column_count; index++) {
            if (index > 0U) {
                fputc(',', printer->output_stream);
            }
            fputs(printer->schema->columns[printer->column_indexes[index]].name, printer->output_stream);
        }
        fputc('\n', printer->output_stream);
        printer->header_written = true;
    }

    for (index = 0U; index < printer->column_count; index++) {
        if (index > 0U) {
            fputc(',', printer->output_stream);
        }
        fputs(row->values[printer->column_indexes[index]], printer->output_stream);
    }
    fputc('\n', printer->output_stream);

    return true;
}

/* INSERT 문을 검증 후 저장 계층에 전달해 실제로 적재한다. */
static SqlStatus execute_insert(
    ExecutionContext *context,
    const InsertStatement *insert_stmt,
    FILE *error_stream,
    bool *did_error
)
{
    TableSchema schema;
    SqlStatus status;
    char **ordered_values;

    status = schema_load(context->schema_dir, insert_stmt->table_name, &schema);
    if (status != SQL_STATUS_OK) {
        write_error(error_stream, status);
        *did_error = true;
        return status;
    }

    ordered_values = NULL;
    status = build_insert_values(&schema, insert_stmt, &ordered_values);
    if (status != SQL_STATUS_OK) {
        schema_destroy(&schema);
        write_error(error_stream, status);
        *did_error = true;
        return status;
    }

    status = storage_insert_row(&context->storage, &schema, ordered_values);
    if (status != SQL_STATUS_OK) {
        write_error(error_stream, status);
        *did_error = true;
    }

    free_ordered_values(ordered_values, schema.column_count);
    schema_destroy(&schema);
    return status;
}

/* SELECT 문을 실행해 스캔 결과를 출력 스트림으로 내보낸다. */
static SqlStatus execute_select(
    ExecutionContext *context,
    const SelectStatement *select_stmt,
    FILE *output_stream,
    FILE *error_stream,
    bool *did_error
)
{
    TableSchema schema;
    SelectPrinterContext printer;
    SqlStatus status;

    status = schema_load(context->schema_dir, select_stmt->table_name, &schema);
    if (status != SQL_STATUS_OK) {
        write_error(error_stream, status);
        *did_error = true;
        return status;
    }

    memset(&printer, 0, sizeof(printer));
    printer.output_stream = output_stream;
    printer.schema = &schema;

    status = build_select_columns(&schema, select_stmt, &printer.column_indexes, &printer.column_count);
    if (status != SQL_STATUS_OK) {
        schema_destroy(&schema);
        write_error(error_stream, status);
        *did_error = true;
        return status;
    }

    status = storage_scan_rows(&context->storage, &schema, print_selected_row, &printer);
    if (status != SQL_STATUS_OK) {
        write_error(error_stream, status);
        *did_error = true;
    }

    free(printer.column_indexes);
    schema_destroy(&schema);
    return status;
}

/* AST 문장 타입에 따라 INSERT/SELECT 실행 함수로 분기한다. */
SqlStatus execute_statement(
    ExecutionContext *context,
    const Statement *statement,
    FILE *output_stream,
    FILE *error_stream,
    bool *did_error
)
{
    if (statement->type == STATEMENT_INSERT) {
        return execute_insert(context, &statement->as.insert_stmt, error_stream, did_error);
    }

    if (statement->type == STATEMENT_SELECT) {
        return execute_select(context, &statement->as.select_stmt, output_stream, error_stream, did_error);
    }

    write_error(error_stream, SQL_STATUS_INVALID_QUERY);
    *did_error = true;
    return SQL_STATUS_INVALID_QUERY;
}
