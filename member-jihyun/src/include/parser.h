#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

#include "ast.h"
#include "status.h"

SqlStatus split_sql_statements(const char *sql_text, char ***statements, size_t *statement_count);
void free_statement_list(char **statements, size_t statement_count);
SqlStatus parse_statement(const char *sql_text, Statement *out_statement);

#endif
