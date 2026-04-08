# Mini SQL Processor Usage

## 위치

- 구현 코드: `member-dragon/src/`
- 실행 파일: `member-dragon/src/sql_processor`
- 실행용 폴더: `member-dragon/playground/`
- 공통 테스트: `common/tests/`
- 공통 스키마: `common/schema/`

## 빌드

```bash
cd member-dragon/src
make
```

빌드가 끝나면 `sql_processor` 실행 파일이 생성됩니다.

## 실행

```bash
cd member-dragon/playground
../src/sql_processor <sql_file>
```

예시:

```bash
../src/sql_processor sample.sql
```

주의:

- 프로그램은 **현재 작업 디렉토리**의 `*.schema` 파일을 읽습니다.
- 공통 테스트 스크립트를 사용할 때는 스크립트가 임시 디렉토리로 스키마를 복사해 주므로 별도 준비가 필요 없습니다.
- 직접 실행할 때는 실행 위치에 `.schema` 파일이 있어야 합니다.
- 기본 실행 예제로 `member-dragon/playground/`에 `users.schema`, `products.schema`, `sample.sql`을 준비해 두었습니다.
- 실행 중 `INSERT`가 일어나면 현재 디렉토리에 `users.data`, `products.data` 같은 데이터 파일이 생성될 수 있습니다.
- `member-dragon/playground/reset.sh`를 실행하면 생성된 `.data` 파일만 지우고 다시 시작할 수 있습니다.
- `member-dragon/playground/run_case.sh`와 `run_suite.sh`로 테스트 케이스를 바로 실행할 수 있습니다.

## 지원 SQL

현재 지원 범위는 아래 두 가지입니다.

```sql
INSERT INTO users (name, age, major) VALUES ('김민준', 25, '컴퓨터공학');
SELECT * FROM users;
SELECT name, major FROM users;
```

지원하지 않는 예:

- `WHERE`
- `UPDATE`
- `DELETE`
- `JOIN`

## 출력 규칙

- `INSERT` 성공 시 출력 없음
- `SELECT` 성공 시:
  - 첫 줄에 헤더 출력
  - 이후 데이터 행 출력
- 빈 결과 `SELECT`는 출력 없음
- 에러는 stderr로 아래 exact string 중 하나를 출력

```text
ERROR: invalid query
ERROR: table not found
ERROR: column count does not match value count
ERROR: file open failed
```

## 테스트

프로젝트 루트에서 실행합니다.

공개 테스트:

```bash
./common/scripts/run_tests.sh ./member-dragon/src/sql_processor public
```

히든 테스트:

```bash
./common/scripts/run_tests.sh ./member-dragon/src/sql_processor hidden
```

`playground`에서 케이스 하나만 실행:

```bash
cd member-dragon/playground
./run_case.sh public 01_basic
./run_case.sh hidden 06_comma_in_value
```

`playground`에서 공개/히든 전체 실행:

```bash
cd member-dragon/playground
./run_suite.sh public
./run_suite.sh hidden
```

## 직접 실행 예시

실행:

```bash
cd member-dragon/src
make

cd ../playground
./reset.sh
../src/sql_processor sample.sql
```

예시 SQL 파일:

```sql
INSERT INTO users (name, age, major) VALUES ('김민준', 25, '컴퓨터공학');
INSERT INTO users (name, age, major) VALUES ('이서연', 22, '경영학');
SELECT * FROM users;
```

예상 stdout:

```text
name,age,major
김민준,25,컴퓨터공학
이서연,22,경영학
```

실행 후에는 `member-dragon/playground/users.data` 파일이 생성되어, 다음 `SELECT`는 그 파일에서 데이터를 읽습니다.

## 정리

- 다시 빌드: `cd member-dragon/src && make`
- 빌드 산출물 삭제: `cd member-dragon/src && make clean`
- 실행 데이터를 초기화하고 다시 시작: `cd member-dragon/playground && ./reset.sh`
- 테스트 기준과 세부 규약: `common/docs/`, `member-dragon/harness-starter-pack/`
