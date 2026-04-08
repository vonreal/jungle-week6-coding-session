# WHERE 단일 조건 테스트 케이스 명세

이 문서는 `run_where_tests.sh`에서 실행하는 단일 `WHERE` 조건 테스트를 설명합니다.

현재 기대하는 확장 범위는 `SELECT ... FROM <table> WHERE <column> = <value>` 한 가지입니다.

## 1) `where_single_int_condition_filters_rows`
- 시나리오:
  - `users`에 여러 행을 넣고 `WHERE age = 25`로 조회합니다.
- 의도:
  - 정수 컬럼 비교로 원하는 행만 필터링되는지 확인합니다.
- 검사:
  - 종료코드 `0`
  - `stderr` 비어 있음
  - `stdout`에 조건과 일치하는 행만 출력됨

## 2) `where_single_string_condition_filters_rows`
- 시나리오:
  - 문자열 컬럼 `major`에 대해 `WHERE major = '컴퓨터공학'`을 수행합니다.
- 의도:
  - 문자열 비교 조건도 동일한 방식으로 처리되는지 확인합니다.
- 검사:
  - 종료코드 `0`
  - `stderr` 비어 있음
  - `stdout`이 기대한 한 행만 포함함

## 3) `where_invalid_column_reports_invalid_query`
- 시나리오:
  - 존재하지 않는 컬럼으로 `WHERE` 조건을 작성합니다.
- 의도:
  - 잘못된 조건 컬럼이 `invalid query`로 거절되는지 확인합니다.
- 검사:
  - 종료코드 `1`
  - `stdout` 비어 있음
  - `stderr`가 `ERROR: invalid query`와 정확히 일치

## 4) `where_error_does_not_stop_next_statement`
- 시나리오:
  - 잘못된 `WHERE` 조회 후 정상 `INSERT`와 `SELECT`를 이어서 수행합니다.
- 의도:
  - `WHERE` 처리 오류도 기존 에러 처리 규칙을 그대로 따르는지 확인합니다.
- 검사:
  - 종료코드 `1`
  - `stderr`는 첫 문장의 에러 1줄만 포함함
  - `stdout`은 후속 성공 문장 결과를 포함함
