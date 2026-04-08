# Mini SQL Parser — Codex Implementation Plan

## Objective

Build a C-based SQL processor in `member-dragon/src/`. It reads a `.sql` file, parses and executes `INSERT` and `SELECT` statements against file-backed tables whose schemas are loaded from `.schema` JSON files in the current working directory.

**IMPORTANT CONSTRAINTS:**
- Only create/modify files inside `member-dragon/`
- Do NOT implement networking or sockets — use a lightweight file-backed table store only
- Do NOT modify anything in `common/`
- All error messages must be **exact strings** (case-sensitive, no extra text)
- Design with a vtable (function pointer table) pattern for the storage layer so it can be swapped later

## Review-Driven Corrections

The initial plan had a few gaps that would make the implementation brittle. These are corrected here and recorded so the reason stays visible during implementation.

| Issue found in review | What was wrong | Plan correction |
|---|---|---|
| `ResultSet` ownership was unclear | The original `char **headers` / `char ***rows` design did not define who allocates or frees per-SELECT buffers, which would likely leak memory or create invalid frees | Switch `ResultSet` to fixed-capacity buffers owned by the caller struct itself so one `SELECT` does not create extra ownership rules |
| Schema loading failure path was unchecked | `load_schemas()` could return `-1`, but `main()` still tried to initialize storage, making startup failure undefined | `main()` must check schema load result before storage init and fail fast with the standard file-open error path |
| Plan said “split statements” but sample loop read one line at a time | A line-oriented loop would only work because current tests put one SQL statement per line; it would not match the stated design goal | `main()` will accumulate file contents and split statements on semicolons that are outside quotes |
| INSERT column list was treated as informational only | Ignoring the column list could silently write data into the wrong schema positions when names or order differ | Parser will keep the INSERT column list, and executor will validate it before storage insert. Count mismatches follow the project’s `column count does not match value count` error path; same-count name/order mismatches are treated as invalid queries |
| Original storage direction violated the assignment brief | The first draft focused on shared tests and chose in-memory rows, but the assignment explicitly says INSERT writes files and SELECT reads files | Replace the backend with a file-backed `StorageOps` implementation that writes `<table>.data` files in the current working directory |

---

## File Structure to Create

```
member-dragon/src/
├── Makefile
├── types.h             # Shared types: Command, TableDef, ResultSet, StorageOps vtable
├── main.c              # Entry point: read .sql file, split statements, run loop
├── schema.h / schema.c # Load *.schema JSON files from cwd into TableDef[]
├── parser.h / parser.c # Parse SQL string → Command struct
├── storage.h           # StorageOps vtable interface (no implementation)
├── file_storage.h / file_storage.c      # File-backed StorageOps implementation
├── executor.h / executor.c              # Execute Command via StorageOps
```

---

## 1. Types (`types.h`)

```c
#ifndef TYPES_H
#define TYPES_H

#define MAX_NAME_LEN 256
#define MAX_COLUMNS 32
#define MAX_VALUES 32
#define MAX_ROWS 1024
#define MAX_TABLES 16

typedef enum { CMD_NONE, CMD_INSERT, CMD_SELECT } CommandType;

typedef struct {
    char name[MAX_NAME_LEN];
    char type[MAX_NAME_LEN]; // "string" or "int"
} ColumnDef;

typedef struct {
    char name[MAX_NAME_LEN];
    ColumnDef columns[MAX_COLUMNS];
    int column_count;
} TableDef;

typedef struct {
    CommandType type;
    char table_name[MAX_NAME_LEN];
    // For INSERT:
    char insert_columns[MAX_COLUMNS][MAX_NAME_LEN];
    int insert_column_count;
    char values[MAX_VALUES][MAX_NAME_LEN];
    int value_count;
    // For SELECT:
    char columns[MAX_COLUMNS][MAX_NAME_LEN];
    int column_count;
    int is_select_all; // 1 if SELECT *
} Command;

typedef struct {
    char headers[MAX_COLUMNS][MAX_NAME_LEN];
    int header_count;
    char rows[MAX_ROWS][MAX_COLUMNS][MAX_NAME_LEN];
    int row_count;
} ResultSet;

// Storage vtable — abstract interface for data backends
typedef struct StorageOps {
    void *ctx; // opaque context pointer
    int  (*init)(void *ctx, TableDef *tables, int table_count);
    int  (*insert)(void *ctx, const char *table, const char values[][MAX_NAME_LEN], int value_count);
    int  (*select_rows)(void *ctx, const char *table, const char columns[][MAX_NAME_LEN], int col_count,
                        int select_all, ResultSet *out);
    void (*destroy)(void *ctx);
} StorageOps;

typedef enum {
    ERR_NONE = 0,
    ERR_INVALID_QUERY,
    ERR_TABLE_NOT_FOUND,
    ERR_COLUMN_MISMATCH,
    ERR_FILE_OPEN
} ErrorCode;

#endif
```

---

## 2. Schema Loader (`schema.c`)

**Behavior:**
- Scan current working directory for files matching `*.schema`
- Parse each file as JSON with this structure:
```json
{
  "table": "users",
  "columns": [
    { "name": "name", "type": "string" },
    { "name": "age", "type": "int" },
    { "name": "major", "type": "string" }
  ]
}
```
- Return an array of `TableDef` and the count
- **No external JSON library** — hand-parse with string functions (strstr, sscanf, etc.)

**Known schemas (for reference, but code must parse dynamically):**

Table `users`: columns = `[name:string, age:int, major:string]`
Table `products`: columns = `[name:string, price:int, category:string]`

**Interface:**
```c
int load_schemas(const char *dir, TableDef *tables, int max_tables);
// Returns number of tables loaded, or -1 on error
```

---

## 3. Parser (`parser.c`)

**Interface:**
```c
int parse_sql(const char *sql_line, Command *cmd);
// Returns 0 on success, -1 on parse error
// Sets cmd->type = CMD_NONE for blank lines
```

**Supported SQL syntax:**
```
INSERT INTO <table> (<col1>, <col2>, ...) VALUES (<val1>, <val2>, ...);
SELECT <col1>, <col2>, ... FROM <table>;
SELECT * FROM <table>;
```

**Critical parsing rules:**
1. **Keywords are case-insensitive**: `INSERT`, `insert`, `Insert` all valid. Use `strcasecmp`.
2. **Values are in single quotes or unquoted integers**: `'김민준'`, `25`, `'컴퓨터공학'`
3. **Single-quoted values may contain spaces**: `'김 민준'` → `김 민준`
4. **Single-quoted values may contain commas**: `'키보드, 마우스 세트'` → `키보드, 마우스 세트`
5. **Blank lines (whitespace only) must be skipped** — set `cmd->type = CMD_NONE`
6. **Trailing semicolons** should be handled (may or may not be present)
7. **INSERT includes a full column list** in parentheses — the implementation will parse and validate it against the schema to avoid silent positional mistakes
   - If the column count does not match the schema’s full-row INSERT shape, report `ERROR: column count does not match value count`
   - If the count matches but names/order do not, report `ERROR: invalid query`

**Value extraction algorithm:**
When parsing the VALUES clause `(val1, val2, ...)`:
- Iterate character by character
- If `'` is found, read until the next `'` — everything between is one value (including commas and spaces)
- If not in quotes, split on `,` and trim whitespace
- Strip surrounding quotes from string values before storing

The parser still works on **one SQL statement at a time**. Multi-line files or multiple statements per line are handled by `main()` before `parse_sql()` is called.

---

## 4. File Storage (`file_storage.c`)

Implements `StorageOps` vtable using per-table data files in the current working directory.

**Interface:**
```c
StorageOps *file_storage_create(void);
// Returns a new StorageOps with ctx allocated, function pointers set
```

**Internal data structure:**
```c
typedef struct {
    TableDef tables[MAX_TABLES];
    int table_count;
    char data_dir[MAX_PATH_LEN];
} FileStore;
```

**Behavior:**
- `init`: Copy TableDef array into `FileStore` and capture the current working directory as the data directory
- `insert`: Find table by name → if not found return ERR_TABLE_NOT_FOUND. Check value_count == column_count → if mismatch return ERR_COLUMN_MISMATCH. Append one encoded row to `<table>.data`
- `select_rows`: Find table by name → if not found return ERR_TABLE_NOT_FOUND. If the data file does not exist yet, treat it as an empty table. Otherwise read `<table>.data`, decode rows, validate requested columns, and fill the caller-owned `ResultSet`
- `destroy`: Free the allocated store context

---

## 5. Executor (`executor.c`)

**Interface:**
```c
int execute_command(Command *cmd, StorageOps *ops, TableDef *tables, int table_count);
// Returns 0 on success, -1 on error (error already printed to stderr)
```

**Behavior for INSERT:**
1. Validate that the parsed INSERT column list matches the project’s full-row schema shape
   - count mismatch → `ERROR: column count does not match value count`
   - same-count name/order mismatch → `ERROR: invalid query`
2. Call `ops->insert(ops->ctx, cmd->table_name, cmd->values, cmd->value_count)`
3. If returns ERR_TABLE_NOT_FOUND → `fprintf(stderr, "ERROR: table not found\n")`
4. If returns ERR_COLUMN_MISMATCH → `fprintf(stderr, "ERROR: column count does not match value count\n")`
5. Success → no output (silent)

**Behavior for SELECT:**
1. Call `ops->select_rows(ops->ctx, cmd->table_name, columns, count, is_select_all, &result)`
2. If returns ERR_TABLE_NOT_FOUND → `fprintf(stderr, "ERROR: table not found\n")`
3. If returns ERR_INVALID_QUERY → `fprintf(stderr, "ERROR: invalid query\n")`
4. If result has 0 rows → **no output at all** (no headers either)
5. If result has rows:
   - Print header line: column names joined by `,` followed by `\n`
   - Print each row: values joined by `,` followed by `\n`
   - Output goes to **stdout**

---

## 6. Main (`main.c`)

```c
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "ERROR: file open failed\n");
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "ERROR: file open failed\n");
        return 1;
    }

    // 1. Load schemas from current directory
    TableDef tables[MAX_TABLES];
    int table_count = load_schemas(".", tables, MAX_TABLES);

    // 2. Initialize storage (file backend)
    StorageOps *ops = file_storage_create();
    ops->init(ops->ctx, tables, table_count);

    if (table_count < 0) {
        fprintf(stderr, "ERROR: file open failed\n");
        fclose(fp);
        return 1;
    }

    // 3. Read and execute each statement
    int has_error = 0;
    char statement[8192];
    // Accumulate chars and split on semicolons that are outside quotes.
    while (read_next_statement(fp, statement, sizeof(statement))) {
        Command cmd;
        if (parse_sql(statement, &cmd) != 0) {
            fprintf(stderr, "ERROR: invalid query\n");
            has_error = 1;
            continue;
        }
        if (cmd.type == CMD_NONE) continue; // blank line

        if (execute_command(&cmd, ops, tables, table_count) != 0) {
            has_error = 1;
        }
    }

    // 4. Cleanup
    fclose(fp);
    ops->destroy(ops->ctx);
    free(ops);

    return has_error ? 1 : 0;
}
```

---

## 7. Makefile

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = sql_processor
SRCS = main.c schema.c parser.c file_storage.c executor.c
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
```

---

## 8. Exact Error Messages (MUST match exactly)

These are the **only** error strings the program may produce. They go to **stderr**.

| Condition | Exact stderr output |
|---|---|
| SQL file cannot be opened | `ERROR: file open failed\n` |
| SQL cannot be parsed | `ERROR: invalid query\n` |
| Table name not in schemas | `ERROR: table not found\n` |
| INSERT value count != schema column count | `ERROR: column count does not match value count\n` |
| SELECT references non-existent column | `ERROR: invalid query\n` |

Rules:
- One error per SQL statement max
- After an error, continue to the next statement
- Exit code 0 if all succeeded, 1 if any error occurred

---

## 9. Test Cases (All Must Pass)

### Public Tests (6)

**01_basic** — 2 INSERTs + SELECT *
```
Input:  INSERT INTO users (name, age, major) VALUES ('김민준', 25, '컴퓨터공학');
        INSERT INTO users (name, age, major) VALUES ('이서연', 22, '경영학');
        SELECT * FROM users;
stdout: name,age,major
        김민준,25,컴퓨터공학
        이서연,22,경영학
stderr: (empty)
```

**02_select_columns** — SELECT specific columns
```
Input:  INSERT INTO users ... VALUES ('김민준', 25, '컴퓨터공학');
        INSERT INTO users ... VALUES ('이서연', 22, '경영학');
        INSERT INTO users ... VALUES ('박지호', 23, '물리학');
        SELECT name, major FROM users;
stdout: name,major
        김민준,컴퓨터공학
        이서연,경영학
        박지호,물리학
```

**03_multi_table** — Cross-table operations
```
Input:  INSERT INTO users ... VALUES ('김민준', 25, '컴퓨터공학');
        INSERT INTO products ... VALUES ('노트북', 1500000, '전자기기');
        INSERT INTO products ... VALUES ('키보드', 89000, '주변기기');
        SELECT * FROM products;
        SELECT name FROM users;
stdout: name,price,category
        노트북,1500000,전자기기
        키보드,89000,주변기기
        name
        김민준
```

**04_incremental** — INSERT-SELECT-INSERT-SELECT (data accumulates)
```
Input:  INSERT INTO users ... VALUES ('김민준', 25, '컴퓨터공학');
        SELECT * FROM users;
        INSERT INTO users ... VALUES ('이서연', 22, '경영학');
        SELECT * FROM users;
stdout: name,age,major
        김민준,25,컴퓨터공학
        name,age,major
        김민준,25,컴퓨터공학
        이서연,22,경영학
```

**05_case_insensitive** — Lowercase keywords
```
Input:  insert into users (name, age, major) values ('김민준', 25, '컴퓨터공학');
        Select * from users;
stdout: name,age,major
        김민준,25,컴퓨터공학
```

**06_many_rows** — 5 INSERTs + SELECT two columns
```
Input:  5x INSERT INTO users ...
        SELECT name, age FROM users;
stdout: name,age
        김민준,25
        이서연,22
        박지호,23
        최유나,21
        정현우,24
```

### Hidden Tests (8)

**hidden/01_empty_table** — SELECT from empty table → no output at all
```
Input:  SELECT * FROM users;
stdout: (empty)
stderr: (empty)
```

**hidden/02_blank_lines** — SQL file has blank lines between statements
```
Input:  INSERT INTO users ... VALUES ('김민준', 25, '컴퓨터공학');
        (blank line)
        INSERT INTO users ... VALUES ('이서연', 22, '경영학');
        (blank line)
        (blank line)
        SELECT * FROM users;
stdout: name,age,major
        김민준,25,컴퓨터공학
        이서연,22,경영학
```

**hidden/03_column_mismatch** — INSERT with wrong number of values
```
Input:  INSERT INTO users (name, age, major) VALUES ('김민준', 25);
        SELECT * FROM users;
stdout: (empty)
stderr: ERROR: column count does not match value count
```

**hidden/04_no_table** — Non-existent table (both INSERT and SELECT fail)
```
Input:  INSERT INTO orders (item, qty) VALUES ('notebook', 3);
        SELECT * FROM orders;
stdout: (empty)
stderr: ERROR: table not found
        ERROR: table not found
```

**hidden/05_spaces_in_value** — Values with spaces inside quotes
```
Input:  INSERT INTO users ... VALUES ('김 민준', 25, '컴퓨터 공학과');
        INSERT INTO users ... VALUES ('이서연', 22, '경영학');
        SELECT * FROM users;
stdout: name,age,major
        김 민준,25,컴퓨터 공학과
        이서연,22,경영학
```

**hidden/06_comma_in_value** — Value with comma inside quotes
```
Input:  INSERT INTO products ... VALUES ('키보드, 마우스 세트', 45000, '주변기기');
        SELECT * FROM products;
stdout: name,price,category
        키보드, 마우스 세트,45000,주변기기
```

**hidden/07_invalid_column** — SELECT non-existent column
```
Input:  INSERT INTO users ... VALUES ('김민준', 25, '컴퓨터공학');
        SELECT name, email FROM users;
stdout: (empty)
stderr: ERROR: invalid query
```

**hidden/08_mixed_errors** — Multiple errors + recovery
```
Input:  INSERT INTO users ... VALUES ('김민준', 25, '컴퓨터공학');    ← OK
        INSERT INTO users (name, age) VALUES ('이서연', 22);          ← column mismatch
        INSERT INTO orders (item, qty) VALUES ('notebook', 3);         ← table not found
        INSERT INTO users ... VALUES ('박지호', 23, '물리학');          ← OK
        SELECT * FROM users;                                           ← OK (2 rows)
stdout: name,age,major
        김민준,25,컴퓨터공학
        박지호,23,물리학
stderr: ERROR: column count does not match value count
        ERROR: table not found
exit:   1
```

---

## 10. Build & Test Commands

```bash
# Build
cd member-dragon/src
make

# Run public tests (from project root)
./common/scripts/run_tests.sh ./member-dragon/src/sql_processor public

# Run hidden tests (from project root)
./common/scripts/run_tests.sh ./member-dragon/src/sql_processor hidden
```

The test runner:
1. Creates a temp directory per test
2. Copies `*.schema` files from `common/schema/` into it
3. Copies the `.sql` file into it
4. `cd`s into the temp dir and runs `./sql_processor <file>.sql`
5. Compares stdout and stderr against `.expected` and `.expected_err` files
6. Reports PASS/FAIL

---

## 11. Harness Starter Pack Documentation Updates

After implementation, update these files inside `member-dragon/harness-starter-pack/`:

| File | Change |
|---|---|
| `AGENTS.md` | Add mini-sql-parser project context, 3-session roadmap |
| `ARCHITECTURE.md` | Document vtable architecture, module boundaries |
| `docs/design-docs/golden-principles.md` | C-specific: memory safety, interface stability, error recovery |
| `docs/design-docs/index.md` | Update document list for SQL parser |
| `docs/product-specs/index.md` | Session 1 feature spec (INSERT, SELECT, errors) |
| `docs/product-specs/feature-template.md` | Concrete SQL parser feature spec |
| `docs/exec-plans/active/current-plan.md` | Current implementation status |
| `docs/design-docs/domain-rules.md` | **NEW** — SQL dialect rules, value parsing, error priority |

Also update `member-dragon/` root docs:
| File | Change |
|---|---|
| `decisions.md` | vtable pattern rationale, module separation, memory management |
| `prompts.md` | AI usage log |
| `trouble.md` | Issues encountered and solutions |

---

## 12. Key Edge Cases to Handle

1. **Empty table SELECT** → zero output (no headers, no rows, no newline)
2. **Blank lines in SQL file** → skip silently, do not error
3. **Commas inside single-quoted values** → must not split on them
4. **Spaces inside single-quoted values** → preserve exactly
5. **Integer values** → no quotes, store as string internally, output as-is
6. **Multiple SELECTs** → each prints its own header line
7. **Error recovery** → after error on one statement, continue processing next
8. **Exit code** → 1 if ANY error occurred across all statements, 0 otherwise
9. **INSERT column list** → present in SQL but values are positional (match schema order)
