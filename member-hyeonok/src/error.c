#include "error.h"

#include <stdio.h>

const char *app_error_message(AppError error)
{
    if (error == APP_ERROR_INVALID_QUERY) {
        return "ERROR: invalid query";
    }

    if (error == APP_ERROR_TABLE_NOT_FOUND) {
        return "ERROR: table not found";
    }

    if (error == APP_ERROR_COLUMN_COUNT_MISMATCH) {
        return "ERROR: column count does not match value count";
    }

    if (error == APP_ERROR_FILE_OPEN_FAILED) {
        return "ERROR: file open failed";
    }

    return "";
}

void print_app_error(AppError error)
{
    const char *message;

    message = app_error_message(error);
    if (message[0] == '\0') {
        return;
    }

    /*
     * 모든 표준 에러 출력은 이 함수 한 곳으로 모은다.
     * 이렇게 해야 철자와 공백이 어긋나는 실수를 줄일 수 있다.
     */
    fprintf(stderr, "%s\n", message);
}

AppError app_error_from_parse_result(ParseResult parse_result)
{
    if (parse_result == PARSE_INVALID_QUERY) {
        return APP_ERROR_INVALID_QUERY;
    }

    if (parse_result == PARSE_INTERNAL_ERROR) {
        return APP_ERROR_FILE_OPEN_FAILED;
    }

    return APP_ERROR_NONE;
}

AppError app_error_from_execute_result(ExecuteResult execute_result)
{
    if (execute_result == EXECUTE_INVALID_QUERY) {
        return APP_ERROR_INVALID_QUERY;
    }

    if (execute_result == EXECUTE_TABLE_NOT_FOUND) {
        return APP_ERROR_TABLE_NOT_FOUND;
    }

    if (execute_result == EXECUTE_COLUMN_COUNT_MISMATCH) {
        return APP_ERROR_COLUMN_COUNT_MISMATCH;
    }

    if (execute_result == EXECUTE_FILE_ERROR) {
        return APP_ERROR_FILE_OPEN_FAILED;
    }

    return APP_ERROR_NONE;
}
