const templates = {
  insert: [
    "INSERT INTO users (name, age, major) VALUES ('김민준', 25, '컴퓨터공학');",
    "INSERT INTO users (name, age, major) VALUES ('이서연', 22, '경영학');"
  ].join("\n"),
  select: "SELECT name, age FROM users;",
  error: "SELECT unknown_col FROM users;"
};

const schemas = {
  users: ["name", "age", "major"],
  products: ["name", "category", "price"]
};

const buttons = Array.from(document.querySelectorAll(".demo-btn"));
const sqlInput = document.getElementById("sqlInput");
const stdoutOutput = document.getElementById("stdoutOutput");
const stderrOutput = document.getElementById("stderrOutput");
const runBtn = document.getElementById("runBtn");
const clearBtn = document.getElementById("clearBtn");
const resetBtn = document.getElementById("resetBtn");
const toastContainer = document.getElementById("toastContainer");

let db = createEmptyDb();

function createEmptyDb() {
  return {
    users: [],
    products: []
  };
}

function showToast(message, type = "info") {
  if (!toastContainer) {
    return;
  }

  const toast = document.createElement("div");
  toast.className = `toast ${type}`;
  toast.textContent = message;
  toastContainer.appendChild(toast);

  setTimeout(() => {
    toast.style.opacity = "0";
    toast.style.transform = "translateY(-4px)";
    setTimeout(() => toast.remove(), 180);
  }, 1800);
}

function isIdentifier(value) {
  return /^[A-Za-z_][A-Za-z0-9_]*$/.test(value);
}

function splitStatements(sqlText) {
  const statements = [];
  let inQuotes = false;
  let start = 0;

  for (let i = 0; i < sqlText.length; i += 1) {
    const ch = sqlText[i];

    if (ch === "'") {
      if (inQuotes && sqlText[i + 1] === "'") {
        i += 1;
        continue;
      }
      inQuotes = !inQuotes;
      continue;
    }

    if (!inQuotes && ch === ";") {
      const part = sqlText.slice(start, i).trim();
      if (part) {
        statements.push(part);
      }
      start = i + 1;
    }
  }

  if (inQuotes) {
    throw new Error("invalid query");
  }

  const tail = sqlText.slice(start).trim();
  if (tail) {
    statements.push(tail);
  }

  return statements;
}

function splitCommaList(text) {
  const items = [];
  let inQuotes = false;
  let tokenStart = 0;

  for (let i = 0; i <= text.length; i += 1) {
    const ch = text[i];

    if (ch === "'") {
      if (inQuotes && text[i + 1] === "'") {
        i += 1;
        continue;
      }
      inQuotes = !inQuotes;
      continue;
    }

    if ((ch === "," && !inQuotes) || i === text.length) {
      const token = text.slice(tokenStart, i).trim();
      if (!token) {
        throw new Error("invalid query");
      }
      items.push(token);
      tokenStart = i + 1;
    }
  }

  if (inQuotes) {
    throw new Error("invalid query");
  }

  return items;
}

function decodeValue(raw) {
  const value = raw.trim();

  if (value.startsWith("'") && value.endsWith("'") && value.length >= 2) {
    const inner = value.slice(1, -1);
    return inner.replace(/''/g, "'");
  }

  return value;
}

function parseInsert(statement) {
  const match = statement.match(/^\s*insert\s+into\s+([A-Za-z_][A-Za-z0-9_]*)\s*\((.+)\)\s*values\s*\((.+)\)\s*$/i);
  if (!match) {
    throw new Error("invalid query");
  }

  const table = match[1].toLowerCase();
  const columns = splitCommaList(match[2]).map((item) => item.toLowerCase());
  const values = splitCommaList(match[3]).map(decodeValue);

  return { type: "insert", table, columns, values };
}

function parseSelect(statement) {
  const match = statement.match(/^\s*select\s+(.+?)\s+from\s+([A-Za-z_][A-Za-z0-9_]*)\s*$/i);
  if (!match) {
    throw new Error("invalid query");
  }

  const columnPart = match[1].trim();
  const table = match[2].toLowerCase();
  const selectAll = columnPart === "*";
  const columns = selectAll ? [] : splitCommaList(columnPart).map((item) => item.toLowerCase());

  return { type: "select", table, selectAll, columns };
}

function parseStatement(statement) {
  const lower = statement.trim().toLowerCase();

  if (lower.startsWith("insert")) {
    return parseInsert(statement);
  }

  if (lower.startsWith("select")) {
    return parseSelect(statement);
  }

  throw new Error("invalid query");
}

function executeInsert(ast) {
  const schema = schemas[ast.table];
  if (!schema) {
    throw new Error("table not found");
  }

  if (ast.columns.length !== ast.values.length || ast.columns.length !== schema.length) {
    throw new Error("column count does not match value count");
  }

  const row = {};
  const assigned = new Set();

  for (let i = 0; i < ast.columns.length; i += 1) {
    const col = ast.columns[i];

    if (!isIdentifier(col) || !schema.includes(col) || assigned.has(col)) {
      throw new Error("invalid query");
    }

    row[col] = ast.values[i];
    assigned.add(col);
  }

  db[ast.table].push(row);
}

function executeSelect(ast, stdoutLines) {
  const schema = schemas[ast.table];
  if (!schema) {
    throw new Error("table not found");
  }

  const columns = ast.selectAll ? schema : ast.columns;

  for (const col of columns) {
    if (!isIdentifier(col) || !schema.includes(col)) {
      throw new Error("invalid query");
    }
  }

  const rows = db[ast.table];
  if (rows.length === 0) {
    return;
  }

  stdoutLines.push(columns.join(","));

  for (const row of rows) {
    const line = columns.map((col) => row[col] ?? "").join(",");
    stdoutLines.push(line);
  }
}

function runSql() {
  const sql = sqlInput.value;
  const stdoutLines = [];
  const stderrLines = [];

  try {
    const statements = splitStatements(sql);

    for (const statement of statements) {
      try {
        const ast = parseStatement(statement);

        if (ast.type === "insert") {
          executeInsert(ast);
        } else if (ast.type === "select") {
          executeSelect(ast, stdoutLines);
        } else {
          throw new Error("invalid query");
        }
      } catch (err) {
        stderrLines.push(`ERROR: ${err.message || "invalid query"}`);
      }
    }
  } catch (err) {
    stderrLines.push(`ERROR: ${err.message || "invalid query"}`);
  }

  stdoutOutput.textContent = stdoutLines.join("\n");
  stderrOutput.textContent = stderrLines.join("\n");

  if (stderrLines.length > 0) {
    showToast("SQL 실행 완료: 에러가 포함되어 있습니다.", "warn");
  } else {
    showToast("SQL 실행 완료", "success");
  }
}

function setTemplate(name) {
  sqlInput.value = templates[name];
}

buttons.forEach((button) => {
  button.addEventListener("click", () => {
    buttons.forEach((btn) => btn.classList.remove("is-active"));
    button.classList.add("is-active");
    setTemplate(button.dataset.scenario);
    showToast("템플릿을 불러왔습니다.", "info");
  });
});

runBtn.addEventListener("click", runSql);

sqlInput.addEventListener("keydown", (event) => {
  if ((event.ctrlKey || event.metaKey) && event.key === "Enter") {
    event.preventDefault();
    runSql();
  }
});

clearBtn.addEventListener("click", () => {
  stdoutOutput.textContent = "";
  stderrOutput.textContent = "";
  showToast("출력을 비웠습니다.", "info");
});

resetBtn.addEventListener("click", () => {
  db = createEmptyDb();
  stdoutOutput.textContent = "";
  stderrOutput.textContent = "";
  showToast("데이터를 초기화했습니다.", "warn");
});

setTemplate("insert");
showToast("SQL을 입력하고 Run SQL을 눌러 실행하세요.", "info");
