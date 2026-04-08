#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "ast.h"

typedef enum ExecuteResult {
    EXECUTE_SUCCESS = 0,
    EXECUTE_INVALID_QUERY = 1,
    EXECUTE_TABLE_NOT_FOUND = 2,
    EXECUTE_COLUMN_COUNT_MISMATCH = 3,
    EXECUTE_FILE_ERROR = 4
} ExecuteResult;

/*
 * 파싱된 SQL 문장을 실행한다.
 * 현재 단계에서는 INSERT 저장 로직을 실제로 수행하고,
 * 이후 호출부가 표준 에러 문구로 매핑할 수 있는 결과 코드를 돌려준다.
 */
ExecuteResult execute_sql_statement(const SqlStatement *statement);

#endif
