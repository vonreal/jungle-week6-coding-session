#include "schema.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int has_schema_extension(const char *name) {
    size_t len = strlen(name);
    return len >= 7 && strcmp(name + len - 7, ".schema") == 0;
}

static int read_file_text(const char *path, char *buffer, size_t size) {
    FILE *fp = fopen(path, "r");
    size_t read_len;

    if (fp == NULL) {
        return -1;
    }

    read_len = fread(buffer, 1, size - 1, fp);
    if (ferror(fp)) {
        fclose(fp);
        return -1;
    }

    buffer[read_len] = '\0';
    fclose(fp);
    return 0;
}

static int extract_json_string(const char *key_pos, char *out, size_t out_size) {
    const char *colon = strchr(key_pos, ':');
    const char *start;
    const char *end;
    size_t len;

    if (colon == NULL) {
        return -1;
    }

    start = strchr(colon, '"');
    if (start == NULL) {
        return -1;
    }
    start++;

    end = strchr(start, '"');
    if (end == NULL) {
        return -1;
    }

    len = (size_t)(end - start);
    if (len >= out_size) {
        return -1;
    }

    memcpy(out, start, len);
    out[len] = '\0';
    return 0;
}

static int parse_schema_content(const char *content, TableDef *table) {
    const char *table_key = strstr(content, "\"table\"");
    const char *columns_key = strstr(content, "\"columns\"");
    const char *array_start;
    const char *array_end;
    const char *cursor;

    memset(table, 0, sizeof(*table));

    if (table_key == NULL || extract_json_string(table_key, table->name, sizeof(table->name)) != 0) {
        return -1;
    }

    if (columns_key == NULL) {
        return -1;
    }

    array_start = strchr(columns_key, '[');
    array_end = strchr(columns_key, ']');
    if (array_start == NULL || array_end == NULL || array_end <= array_start) {
        return -1;
    }

    cursor = array_start;
    while (cursor < array_end) {
        const char *name_key = strstr(cursor, "\"name\"");
        const char *type_key;

        if (name_key == NULL || name_key >= array_end) {
            break;
        }

        if (table->column_count >= MAX_COLUMNS) {
            return -1;
        }

        type_key = strstr(name_key, "\"type\"");
        if (type_key == NULL || type_key >= array_end) {
            return -1;
        }

        if (extract_json_string(name_key,
                                table->columns[table->column_count].name,
                                sizeof(table->columns[table->column_count].name)) != 0) {
            return -1;
        }

        if (extract_json_string(type_key,
                                table->columns[table->column_count].type,
                                sizeof(table->columns[table->column_count].type)) != 0) {
            return -1;
        }

        table->column_count++;
        cursor = type_key + 6;
    }

    return table->column_count > 0 ? 0 : -1;
}

int load_schemas(const char *dir, TableDef *tables, int max_tables) {
    DIR *dp = opendir(dir);
    struct dirent *entry;
    int table_count = 0;

    if (dp == NULL) {
        return -1;
    }

    while ((entry = readdir(dp)) != NULL) {
        char path[1024];
        char content[8192];

        if (!has_schema_extension(entry->d_name)) {
            continue;
        }

        if (table_count >= max_tables) {
            closedir(dp);
            return -1;
        }

        if (snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name) >= (int)sizeof(path)) {
            closedir(dp);
            return -1;
        }

        if (read_file_text(path, content, sizeof(content)) != 0) {
            closedir(dp);
            return -1;
        }

        if (parse_schema_content(content, &tables[table_count]) != 0) {
            closedir(dp);
            return -1;
        }

        table_count++;
    }

    closedir(dp);
    return table_count;
}
