# ORDER BY 1컬럼 정렬 테스트 케이스 명세

이 문서는 `run_order_tests.sh`에서 실행하는 1컬럼 `ORDER BY` 테스트를 설명합니다.

현재 기대하는 확장 범위는 `SELECT ... FROM <table> ORDER BY <column>` 한 가지이며, 기본 정렬 방향은 오름차순입니다.

## 1) `order_by_single_column_default_ascending`
- 시나리오:
  - `age` 값이 섞인 여러 행을 넣고 `ORDER BY age`로 조회합니다.
- 의도:
  - 단일 컬럼 기준 기본 오름차순 정렬이 적용되는지 확인합니다.
- 검사:
  - 종료코드 `0`
  - `stderr` 비어 있음
  - `stdout` 행 순서가 정렬 기준과 일치함

## 2) `order_by_sort_key_can_differ_from_projection`
- 시나리오:
  - `SELECT name FROM users ORDER BY age`를 수행합니다.
- 의도:
  - 정렬 기준 컬럼이 출력 컬럼에 없어도 정렬이 가능한지 확인합니다.
- 검사:
  - 종료코드 `0`
  - `stderr` 비어 있음
  - `stdout`에서 `name`만 출력되지만 행 순서는 `age` 기준임

## 3) `order_by_invalid_column_reports_invalid_query`
- 시나리오:
  - 존재하지 않는 컬럼으로 `ORDER BY`를 수행합니다.
- 의도:
  - 정렬 기준 컬럼 검증 실패 시 `invalid query`를 반환하는지 확인합니다.
- 검사:
  - 종료코드 `1`
  - `stdout` 비어 있음
  - `stderr`가 `ERROR: invalid query`와 정확히 일치

## 4) `order_by_error_does_not_stop_next_statement`
- 시나리오:
  - 잘못된 정렬 문장 뒤에 정상 `INSERT`, `SELECT`를 이어서 수행합니다.
- 의도:
  - `ORDER BY` 관련 오류도 기존 실행 루프 규칙을 그대로 따르는지 확인합니다.
- 검사:
  - 종료코드 `1`
  - `stderr`는 첫 문장의 에러 1줄만 포함함
  - `stdout`은 후속 성공 문장 결과를 포함함
