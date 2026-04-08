#include "include/parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "include/utils.h"

/* 문자열 배열의 모든 원소를 해제한 뒤 배열 메모리도 반납한다. */
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

/* split 결과로 만들어진 SQL 문장 배열을 해제한다. */
void free_statement_list(char **statements, size_t statement_count)
{
    free_string_array(statements, statement_count);
}

/* 동적 배열 끝에 토큰 하나를 추가하고 개수를 갱신한다. */
static SqlStatus push_token(char ***items, size_t *count, char *token)
{
    char **next_items;

    next_items = (char **)realloc(*items, (*count + 1U) * sizeof(char *));
    if (next_items == NULL) {
        free(token);
        return SQL_STATUS_INTERNAL_ERROR;
    }

    next_items[*count] = token;
    *items = next_items;
    *count += 1U;
    return SQL_STATUS_OK;
}

/* 식별자에 허용되는 문자(영숫자, 밑줄)인지 검사한다. */
static bool is_identifier_char(char value)
{
    return isalnum((unsigned char)value) || value == '_';
}

/* 식별자 토큰의 앞뒤 공백을 제거하고 소문자로 정규화한다. */
static char *normalize_identifier(char *token)
{
    util_trim_inplace(token);
    util_lowercase_inplace(token);
    return token;
}

/* 따옴표로 감싼 값 토큰을 언이스케이프해 실제 값 문자열로 변환한다. */
static char *decode_value_token(char *token)
{
    size_t source_index;
    size_t target_index;
    char *decoded;
    size_t length;

    util_trim_inplace(token);
    length = strlen(token);
    if (length < 2U || token[0] != '\'' || token[length - 1U] != '\'') {
        return token;
    }

    decoded = (char *)malloc(length - 1U);
    if (decoded == NULL) {
        free(token);
        return NULL;
    }

    target_index = 0U;
    for (source_index = 1U; source_index + 1U < length; source_index++) {
        if (token[source_index] == '\'' && token[source_index + 1U] == '\'' && source_index + 2U < length) {
            decoded[target_index++] = '\'';
            source_index++;
            continue;
        }
        decoded[target_index++] = token[source_index];
    }

    decoded[target_index] = '\0';
    free(token);
    return decoded;
}

/* 따옴표 내부 콤마를 보존하면서 콤마 기준으로 항목 목록을 분리한다. */
static SqlStatus split_comma_list(const char *text, bool decode_values, char ***items, size_t *item_count)
{
    const char *token_start;
    bool in_quotes;
    size_t index;
    SqlStatus status;

    *items = NULL;
    *item_count = 0U;
    token_start = text;
    in_quotes = false;

    for (index = 0U;; index++) {
        char current = text[index];

        if (current == '\'') {
            if (in_quotes && text[index + 1U] == '\'') {
                index++;
                continue;
            }
            in_quotes = !in_quotes;
        }

        if ((current == ',' && !in_quotes) || current == '\0') {
            char *token;

            token = util_trim_copy_range(token_start, text + index);
            if (token == NULL || token[0] == '\0') {
                free(token);
                free_string_array(*items, *item_count);
                return SQL_STATUS_INVALID_QUERY;
            }

            if (decode_values) {
                token = decode_value_token(token);
                if (token == NULL) {
                    free_string_array(*items, *item_count);
                    return SQL_STATUS_INTERNAL_ERROR;
                }
            } else {
                normalize_identifier(token);
            }

            status = push_token(items, item_count, token);
            if (status != SQL_STATUS_OK) {
                free_string_array(*items, *item_count);
                *items = NULL;
                *item_count = 0U;
                return status;
            }

            if (current == '\0') {
                break;
            }
            token_start = text + index + 1U;
        }

        if (current == '\0') {
            break;
        }
    }

    if (in_quotes) {
        free_string_array(*items, *item_count);
        *items = NULL;
        *item_count = 0U;
        return SQL_STATUS_INVALID_QUERY;
    }

    return SQL_STATUS_OK;
}

/* 현재 커서 위치에서 식별자 하나를 추출해 소문자로 반환한다. */
static SqlStatus extract_identifier(const char **cursor, char **identifier)
{
    const char *start;

    *cursor = util_skip_spaces(*cursor);
    start = *cursor;
    while (is_identifier_char(**cursor)) {
        (*cursor)++;
    }

    if (start == *cursor) {
        return SQL_STATUS_INVALID_QUERY;
    }

    *identifier = util_trim_copy_range(start, *cursor);
    if (*identifier == NULL) {
        return SQL_STATUS_INTERNAL_ERROR;
    }
    util_lowercase_inplace(*identifier);
    return SQL_STATUS_OK;
}

/* 괄호 쌍 안의 텍스트 구간을 추출하고 커서를 닫는 괄호 뒤로 이동한다. */
static SqlStatus extract_parenthesized_segment(const char **cursor, char **segment)
{
    const char *start;
    const char *scan;
    int depth;
    bool in_quotes;

    *cursor = util_skip_spaces(*cursor);
    if (**cursor != '(') {
        return SQL_STATUS_INVALID_QUERY;
    }

    start = *cursor + 1;
    scan = start;
    depth = 1;
    in_quotes = false;

    while (*scan != '\0') {
        if (*scan == '\'') {
            if (in_quotes && scan[1] == '\'') {
                scan += 2;
                continue;
            }
            in_quotes = !in_quotes;
        } else if (!in_quotes && *scan == '(') {
            depth++;
        } else if (!in_quotes && *scan == ')') {
            depth--;
            if (depth == 0) {
                *segment = util_trim_copy_range(start, scan);
                if (*segment == NULL) {
                    return SQL_STATUS_INTERNAL_ERROR;
                }
                *cursor = scan + 1;
                return SQL_STATUS_OK;
            }
        }
        scan++;
    }

    return SQL_STATUS_INVALID_QUERY;
}

/* SQL 텍스트를 세미콜론 기준으로 분리해 실행 가능한 문장 리스트를 만든다. */
SqlStatus split_sql_statements(const char *sql_text, char ***statements, size_t *statement_count)
{
    const char *statement_start;
    bool in_quotes;
    size_t index;
    SqlStatus status;

    *statements = NULL;
    *statement_count = 0U;
    statement_start = sql_text;
    in_quotes = false;

    for (index = 0U;; index++) {
        char current = sql_text[index];

        if (current == '\'') {
            if (in_quotes && sql_text[index + 1U] == '\'') {
                index++;
                continue;
            }
            in_quotes = !in_quotes;
        }

        if ((current == ';' && !in_quotes) || current == '\0') {
            char *statement;

            statement = util_trim_copy_range(statement_start, sql_text + index);
            if (statement == NULL) {
                free_statement_list(*statements, *statement_count);
                return SQL_STATUS_INTERNAL_ERROR;
            }

            if (!util_is_blank_string(statement)) {
                status = push_token(statements, statement_count, statement);
                if (status != SQL_STATUS_OK) {
                    free_statement_list(*statements, *statement_count);
                    return status;
                }
            } else {
                free(statement);
            }

            if (current == '\0') {
                break;
            }
            statement_start = sql_text + index + 1U;
        }

        if (current == '\0') {
            break;
        }
    }

    return in_quotes ? SQL_STATUS_INVALID_QUERY : SQL_STATUS_OK;
}

/* INSERT 문을 파싱해 테이블, 컬럼, 값 정보를 AST에 채운다. */
static SqlStatus parse_insert_statement(const char *sql_text, Statement *out_statement)
{
    const char *cursor;
    char *column_segment;
    char *value_segment;
    SqlStatus status;

    memset(out_statement, 0, sizeof(*out_statement));
    out_statement->type = STATEMENT_INSERT;

    cursor = sql_text + strlen("insert");
    cursor = util_skip_spaces(cursor);
    if (!util_case_starts_with(cursor, "into")) {
        return SQL_STATUS_INVALID_QUERY;
    }
    cursor += strlen("into");

    status = extract_identifier(&cursor, &out_statement->as.insert_stmt.table_name);
    if (status != SQL_STATUS_OK) {
        return status;
    }

    status = extract_parenthesized_segment(&cursor, &column_segment);
    if (status != SQL_STATUS_OK) {
        statement_destroy(out_statement);
        return status;
    }

    status = split_comma_list(
        column_segment,
        false,
        &out_statement->as.insert_stmt.columns,
        &out_statement->as.insert_stmt.column_count
    );
    free(column_segment);
    if (status != SQL_STATUS_OK) {
        statement_destroy(out_statement);
        return status;
    }

    cursor = util_skip_spaces(cursor);
    if (!util_case_starts_with(cursor, "values")) {
        statement_destroy(out_statement);
        return SQL_STATUS_INVALID_QUERY;
    }
    cursor += strlen("values");

    status = extract_parenthesized_segment(&cursor, &value_segment);
    if (status != SQL_STATUS_OK) {
        statement_destroy(out_statement);
        return status;
    }

    status = split_comma_list(
        value_segment,
        true,
        &out_statement->as.insert_stmt.values,
        &out_statement->as.insert_stmt.value_count
    );
    free(value_segment);
    if (status != SQL_STATUS_OK) {
        statement_destroy(out_statement);
        return status;
    }

    cursor = util_skip_spaces(cursor);
    if (*cursor != '\0') {
        statement_destroy(out_statement);
        return SQL_STATUS_INVALID_QUERY;
    }

    return SQL_STATUS_OK;
}

/* SELECT 문을 파싱해 조회 컬럼/테이블 정보를 AST에 채운다. */
static SqlStatus parse_select_statement(const char *sql_text, Statement *out_statement)
{
    const char *cursor;
    const char *from_keyword;
    char *select_segment;
    SqlStatus status;

    memset(out_statement, 0, sizeof(*out_statement));
    out_statement->type = STATEMENT_SELECT;

    cursor = sql_text + strlen("select");
    from_keyword = util_find_keyword_outside_quotes(cursor, "from");
    if (from_keyword == NULL) {
        return SQL_STATUS_INVALID_QUERY;
    }

    select_segment = util_trim_copy_range(cursor, from_keyword);
    if (select_segment == NULL) {
        return SQL_STATUS_INTERNAL_ERROR;
    }

    if (strcmp(select_segment, "*") == 0) {
        out_statement->as.select_stmt.select_all = true;
        free(select_segment);
    } else {
        status = split_comma_list(
            select_segment,
            false,
            &out_statement->as.select_stmt.columns,
            &out_statement->as.select_stmt.column_count
        );
        free(select_segment);
        if (status != SQL_STATUS_OK) {
            statement_destroy(out_statement);
            return status;
        }
    }

    cursor = from_keyword + strlen("from");
    status = extract_identifier(&cursor, &out_statement->as.select_stmt.table_name);
    if (status != SQL_STATUS_OK) {
        statement_destroy(out_statement);
        return status;
    }

    cursor = util_skip_spaces(cursor);
    if (*cursor != '\0') {
        statement_destroy(out_statement);
        return SQL_STATUS_INVALID_QUERY;
    }

    return SQL_STATUS_OK;
}

/* 문장 종류를 판별해 INSERT/SELECT 전용 파서로 분기한다. */
SqlStatus parse_statement(const char *sql_text, Statement *out_statement)
{
    const char *cursor;

    memset(out_statement, 0, sizeof(*out_statement));
    cursor = util_skip_spaces(sql_text);

    if (util_case_starts_with(cursor, "insert")) {
        return parse_insert_statement(cursor, out_statement);
    }
    if (util_case_starts_with(cursor, "select")) {
        return parse_select_statement(cursor, out_statement);
    }

    return SQL_STATUS_INVALID_QUERY;
}
