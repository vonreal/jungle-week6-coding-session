#ifndef ERROR_H
#define ERROR_H

#include "executor.h"
#include "parser.h"

typedef enum AppError {
    APP_ERROR_NONE = 0,
    APP_ERROR_INVALID_QUERY = 1,
    APP_ERROR_TABLE_NOT_FOUND = 2,
    APP_ERROR_COLUMN_COUNT_MISMATCH = 3,
    APP_ERROR_FILE_OPEN_FAILED = 4
} AppError;

/*
 * 공통 에러 코드를 표준 에러 문구로 변환한다.
 * 반환 문자열은 상수이므로 호출한 쪽에서 해제하지 않는다.
 */
const char *app_error_message(AppError error);

/*
 * 표준 에러 문구를 stderr로 정확히 한 줄 출력한다.
 * 추가 설명 없이 공통 명세의 문구만 출력한다.
 */
void print_app_error(AppError error);

/*
 * 파서 결과를 상위 공통 에러 코드로 변환한다.
 */
AppError app_error_from_parse_result(ParseResult parse_result);

/*
 * 실행기 결과를 상위 공통 에러 코드로 변환한다.
 */
AppError app_error_from_execute_result(ExecuteResult execute_result);

#endif
