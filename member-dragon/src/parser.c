#include "parser.h"

#include <ctype.h>
#include <string.h>
#include <strings.h>

static void trim_in_place(char *text) {
    char *start = text;
    char *end;

    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';
}

static const char *skip_spaces(const char *text) {
    while (*text != '\0' && isspace((unsigned char)*text)) {
        text++;
    }
    return text;
}

static int is_identifier_char(int ch) {
    return isalnum((unsigned char)ch) || ch == '_';
}

static int keyword_matches(const char *text, const char *keyword) {
    size_t len = strlen(keyword);

    return strncasecmp(text, keyword, len) == 0 &&
           !is_identifier_char((unsigned char)text[len]);
}

static int consume_keyword(const char **cursor, const char *keyword) {
    const char *text = skip_spaces(*cursor);
    size_t len = strlen(keyword);

    if (!keyword_matches(text, keyword)) {
        return 0;
    }

    *cursor = text + len;
    return 1;
}

static int parse_identifier(const char **cursor, char *out, size_t out_size) {
    const char *text = skip_spaces(*cursor);
    size_t len = 0;

    if (!is_identifier_char((unsigned char)*text)) {
        return -1;
    }

    while (is_identifier_char((unsigned char)text[len])) {
        len++;
    }

    if (len >= out_size) {
        return -1;
    }

    memcpy(out, text, len);
    out[len] = '\0';
    *cursor = text + len;
    return 0;
}

static int parse_parenthesized_identifiers(const char **cursor,
                                           char items[][MAX_NAME_LEN],
                                           int *count) {
    const char *text = skip_spaces(*cursor);

    *count = 0;
    if (*text != '(') {
        return -1;
    }
    text++;

    for (;;) {
        text = skip_spaces(text);
        if (*text == ')' || *text == '\0') {
            return -1;
        }

        if (*count >= MAX_COLUMNS ||
            parse_identifier(&text, items[*count], sizeof(items[*count])) != 0) {
            return -1;
        }
        (*count)++;

        text = skip_spaces(text);
        if (*text == ',') {
            text++;
            continue;
        }
        if (*text == ')') {
            text++;
            *cursor = text;
            return 0;
        }
        return -1;
    }
}

static int parse_value_token(const char **cursor, char *out, size_t out_size) {
    const char *text = skip_spaces(*cursor);

    if (*text == '\'') {
        const char *end = strchr(text + 1, '\'');
        size_t len;

        if (end == NULL) {
            return -1;
        }

        len = (size_t)(end - (text + 1));
        if (len >= out_size) {
            return -1;
        }

        memcpy(out, text + 1, len);
        out[len] = '\0';
        *cursor = end + 1;
        return 0;
    }

    {
        const char *end = text;
        char token[MAX_VALUE_LEN];
        size_t len;

        while (*end != '\0' && *end != ',' && *end != ')') {
            end++;
        }

        len = (size_t)(end - text);
        if (len >= sizeof(token)) {
            return -1;
        }

        memcpy(token, text, len);
        token[len] = '\0';
        trim_in_place(token);

        if (token[0] == '\0' || strlen(token) >= out_size) {
            return -1;
        }

        strcpy(out, token);
        *cursor = end;
        return 0;
    }
}

static int parse_values_clause(const char **cursor,
                               char values[][MAX_VALUE_LEN],
                               int *value_count) {
    const char *text = skip_spaces(*cursor);

    *value_count = 0;
    if (*text != '(') {
        return -1;
    }
    text++;

    for (;;) {
        text = skip_spaces(text);
        if (*text == ')' || *text == '\0') {
            return -1;
        }

        if (*value_count >= MAX_VALUES ||
            parse_value_token(&text, values[*value_count], sizeof(values[*value_count])) != 0) {
            return -1;
        }
        (*value_count)++;

        text = skip_spaces(text);
        if (*text == ',') {
            text++;
            continue;
        }
        if (*text == ')') {
            text++;
            *cursor = text;
            return 0;
        }
        return -1;
    }
}

static const char *find_keyword_position(const char *text, const char *keyword) {
    const char *cursor = text;

    while (*cursor != '\0') {
        if ((cursor == text || !is_identifier_char((unsigned char)cursor[-1])) &&
            keyword_matches(cursor, keyword)) {
            return cursor;
        }
        cursor++;
    }

    return NULL;
}

static int parse_select_columns_text(const char *text, Command *cmd) {
    const char *cursor = text;

    cmd->column_count = 0;
    cmd->is_select_all = 0;

    if (strcmp(text, "*") == 0) {
        cmd->is_select_all = 1;
        return 0;
    }

    while (*cursor != '\0') {
        const char *start = cursor;
        const char *end = strchr(cursor, ',');
        char token[MAX_NAME_LEN];
        size_t len;
        size_t i;

        if (cmd->column_count >= MAX_COLUMNS) {
            return -1;
        }

        if (end == NULL) {
            end = cursor + strlen(cursor);
        }

        len = (size_t)(end - start);
        if (len >= sizeof(token)) {
            return -1;
        }

        memcpy(token, start, len);
        token[len] = '\0';
        trim_in_place(token);
        if (token[0] == '\0') {
            return -1;
        }

        for (i = 0; token[i] != '\0'; i++) {
            if (!is_identifier_char((unsigned char)token[i])) {
                return -1;
            }
        }

        strcpy(cmd->columns[cmd->column_count], token);
        cmd->column_count++;

        if (*end == '\0') {
            break;
        }
        cursor = end + 1;
    }

    return cmd->column_count > 0 ? 0 : -1;
}

static int parse_insert(const char *sql, Command *cmd) {
    const char *cursor = sql;

    if (!consume_keyword(&cursor, "INSERT")) {
        return -1;
    }
    if (!consume_keyword(&cursor, "INTO")) {
        return -1;
    }
    if (parse_identifier(&cursor, cmd->table_name, sizeof(cmd->table_name)) != 0) {
        return -1;
    }
    if (parse_parenthesized_identifiers(&cursor, cmd->insert_columns, &cmd->insert_column_count) != 0) {
        return -1;
    }
    if (!consume_keyword(&cursor, "VALUES")) {
        return -1;
    }
    if (parse_values_clause(&cursor, cmd->values, &cmd->value_count) != 0) {
        return -1;
    }

    cursor = skip_spaces(cursor);
    if (*cursor == ';') {
        cursor++;
    }
    cursor = skip_spaces(cursor);
    return *cursor == '\0' ? 0 : -1;
}

static int parse_select(const char *sql, Command *cmd) {
    const char *cursor = sql;
    const char *from_pos;
    char columns_text[1024];
    size_t columns_len;

    if (!consume_keyword(&cursor, "SELECT")) {
        return -1;
    }

    cursor = skip_spaces(cursor);
    from_pos = find_keyword_position(cursor, "FROM");
    if (from_pos == NULL || from_pos == cursor) {
        return -1;
    }

    columns_len = (size_t)(from_pos - cursor);
    if (columns_len >= sizeof(columns_text)) {
        return -1;
    }

    memcpy(columns_text, cursor, columns_len);
    columns_text[columns_len] = '\0';
    trim_in_place(columns_text);
    if (parse_select_columns_text(columns_text, cmd) != 0) {
        return -1;
    }

    cursor = from_pos;
    if (!consume_keyword(&cursor, "FROM")) {
        return -1;
    }
    if (parse_identifier(&cursor, cmd->table_name, sizeof(cmd->table_name)) != 0) {
        return -1;
    }

    cursor = skip_spaces(cursor);
    if (*cursor == ';') {
        cursor++;
    }
    cursor = skip_spaces(cursor);
    return *cursor == '\0' ? 0 : -1;
}

int parse_sql(const char *sql, Command *cmd) {
    char working[8192];
    char *end;

    memset(cmd, 0, sizeof(*cmd));
    cmd->type = CMD_NONE;

    if (strlen(sql) >= sizeof(working)) {
        return -1;
    }

    strcpy(working, sql);
    trim_in_place(working);

    end = working + strlen(working);
    while (end > working && end[-1] == ';') {
        end--;
    }
    *end = '\0';
    trim_in_place(working);

    if (working[0] == '\0') {
        return 0;
    }

    if (keyword_matches(working, "INSERT")) {
        cmd->type = CMD_INSERT;
        return parse_insert(working, cmd);
    }

    if (keyword_matches(working, "SELECT")) {
        cmd->type = CMD_SELECT;
        return parse_select(working, cmd);
    }

    return -1;
}
