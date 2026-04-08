#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

/*
 * 하나의 SQL 문장 문자열을 AST 구조체로 변환한다.
 * INSERT 문장은 실제 필드를 채우고, 그 외 문장은 원문만 보관한다.
 */
int parse_sql_statement(const char *sql_text, SqlStatement *statement);

/*
 * 파서가 채운 SQL 문장 구조체 내부 메모리를 해제한다.
 */
void free_sql_statement(SqlStatement *statement);

#endif
