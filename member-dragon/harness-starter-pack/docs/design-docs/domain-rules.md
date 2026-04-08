# Domain Rules

이 프로젝트는 파일 기반 SQL 처리기다. 규칙은 `common/` 문서와 테스트를 우선한다.

## Scope

- 지원 SQL은 `INSERT`와 `SELECT`만 허용한다.
- `UPDATE`, `DELETE`, `JOIN`, `WHERE`는 범위 밖이다.
- 구현은 파일 기반 저장소를 사용한다.

## Schema Rules

- 스키마는 현재 작업 디렉토리의 `*.schema` 파일에서 읽는다.
- 데이터는 현재 작업 디렉토리의 `<table>.data` 파일에서 읽고 쓴다.
- 현재 기준 테이블은 `users`와 `products`다.
- 테이블 이름을 찾지 못하면 `ERROR: table not found`를 출력한다.

## Parse Rules

- SQL 키워드는 대소문자를 구분하지 않는다.
- `main.c`는 인용부호 바깥의 `;`를 기준으로 statement를 분리하고, `parser.c`는 단일 statement만 해석한다.
- `INSERT`의 값은 positional로 저장하되, 컬럼 목록은 schema 전체 컬럼 목록과 이름/순서를 검증한다.
- quoted value 안의 공백과 콤마는 값의 일부로 취급한다.
- 빈 줄은 무시한다.

## Error Priority

- 문법을 해석할 수 없으면 `ERROR: invalid query`를 출력한다.
- 테이블이 없으면 `ERROR: table not found`를 출력한다.
- `INSERT` 컬럼 수나 값 수가 schema 전체 row와 맞지 않으면 `ERROR: column count does not match value count`를 출력한다.
- `INSERT` 컬럼 이름/순서가 schema와 다르면 `ERROR: invalid query`를 출력한다.
- `SELECT`의 컬럼이 없으면 `ERROR: invalid query`를 출력한다.

## Output Rules

- `INSERT` 성공은 silent다.
- `SELECT`가 빈 결과면 아무것도 출력하지 않는다.
- `SELECT` 결과는 헤더 다음 행들을 모두 stdout에 출력한다.
