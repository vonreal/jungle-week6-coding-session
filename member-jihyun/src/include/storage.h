#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include <stddef.h>

#include "buffer_cache.h"
#include "schema.h"
#include "status.h"

typedef struct RowData {
    size_t value_count;
    char **values;
} RowData;

typedef bool (*RowVisitor)(const RowData *row, void *context);

typedef struct StorageEngine {
    char data_dir[MAX_PATH_LENGTH];
    BufferCache cache;
} StorageEngine;

SqlStatus storage_engine_init(StorageEngine *engine, const char *data_dir);
void storage_engine_destroy(StorageEngine *engine);
SqlStatus storage_insert_row(StorageEngine *engine, const TableSchema *schema, char **values);
SqlStatus storage_scan_rows(StorageEngine *engine, const TableSchema *schema, RowVisitor visitor, void *context);
SqlStatus storage_flush(StorageEngine *engine);
void row_data_destroy(RowData *row);

#endif
