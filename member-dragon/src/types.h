#ifndef TYPES_H
#define TYPES_H

#define MAX_NAME_LEN 256
#define MAX_VALUE_LEN 256
#define MAX_COLUMNS 32
#define MAX_VALUES 32
#define MAX_ROWS 1024
#define MAX_TABLES 16

typedef enum {
    CMD_NONE,
    CMD_INSERT,
    CMD_SELECT
} CommandType;

typedef struct {
    char name[MAX_NAME_LEN];
    char type[MAX_NAME_LEN];
} ColumnDef;

typedef struct {
    char name[MAX_NAME_LEN];
    ColumnDef columns[MAX_COLUMNS];
    int column_count;
} TableDef;

typedef struct {
    CommandType type;
    char table_name[MAX_NAME_LEN];
    char insert_columns[MAX_COLUMNS][MAX_NAME_LEN];
    int insert_column_count;
    char values[MAX_VALUES][MAX_VALUE_LEN];
    int value_count;
    char columns[MAX_COLUMNS][MAX_NAME_LEN];
    int column_count;
    int is_select_all;
} Command;

typedef struct {
    char headers[MAX_COLUMNS][MAX_NAME_LEN];
    int header_count;
    char rows[MAX_ROWS][MAX_COLUMNS][MAX_VALUE_LEN];
    int row_count;
} ResultSet;

typedef struct StorageOps {
    void *ctx;
    int (*init)(void *ctx, const TableDef *tables, int table_count);
    int (*insert)(void *ctx, const char *table, const char values[][MAX_VALUE_LEN], int value_count);
    int (*select_rows)(void *ctx, const char *table,
                       const char columns[][MAX_NAME_LEN], int col_count,
                       int select_all, ResultSet *out);
    void (*destroy)(void *ctx);
} StorageOps;

typedef enum {
    ERR_NONE = 0,
    ERR_INVALID_QUERY,
    ERR_TABLE_NOT_FOUND,
    ERR_COLUMN_MISMATCH,
    ERR_FILE_OPEN
} ErrorCode;

#endif
