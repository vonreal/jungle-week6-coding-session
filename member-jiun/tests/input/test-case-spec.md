# 입력 처리 테스트 케이스 명세

이 문서는 `run_input_tests.sh`에서 실행하는 입력 처리(Input Processing) 테스트를 설명한다.

## 1) `input_blank_lines_are_ignored`
- 시나리오:
  - SQL 파일에 빈 줄이 여러 개 포함된 상태로 `INSERT`, `SELECT`를 실행한다.
- 의도:
  - 빈 줄이 실행 의미를 바꾸지 않고 정상 처리되는지 확인한다.
- 검사:
  - 종료코드 `0`
  - `stderr` 비어 있음
  - `stdout`이 기대 결과와 정확히 일치

## 2) `input_whitespace_around_statements_is_ignored`
- 시나리오:
  - 문장 앞뒤에 공백/탭이 포함된 SQL 파일을 실행한다.
- 의도:
  - 문장 주변 공백이 있어도 SQL 문이 정상 인식되는지 확인한다.
- 검사:
  - 종료코드 `0`
  - `stderr` 비어 있음
  - `stdout`이 기대 결과와 정확히 일치

## 3) `input_multiple_statements_in_one_line`
- 시나리오:
  - 한 줄에 여러 SQL 문장을 `;`로 이어 작성해 실행한다.
- 의도:
  - `;` 기준 문장 분리가 정상 동작하는지 확인한다.
- 검사:
  - 종료코드 `0`
  - `stderr` 비어 있음
  - `stdout`이 기대 결과와 정확히 일치
