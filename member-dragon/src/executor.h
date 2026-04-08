#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "types.h"

int execute_command(const Command *cmd, StorageOps *ops, const TableDef *tables, int table_count);

#endif
