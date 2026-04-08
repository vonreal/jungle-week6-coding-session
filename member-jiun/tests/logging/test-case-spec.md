# 실행 로그 파일 테스트 케이스 명세

이 문서는 `run_logging_tests.sh`에서 실행하는 로그 파일 테스트를 설명합니다.

현재 기대하는 확장 범위는 실행 디렉토리에 `sql_processor.log`를 만들고, 문장별 성공/실패 결과를 append 하는 것입니다.

## 1) `logging_creates_log_file_for_successful_run`
- 시나리오:
  - 성공하는 `INSERT`, `SELECT`를 실행합니다.
- 의도:
  - 실행 후 로그 파일이 생성되고 각 문장 성공 기록이 남는지 확인합니다.
- 검사:
  - 종료코드 `0`
  - `sql_processor.log` 파일 존재
  - `SUCCESS INSERT users`, `SUCCESS SELECT users` 문자열 포함

## 2) `logging_records_error_result`
- 시나리오:
  - 컬럼 수가 맞지 않는 잘못된 `INSERT`를 실행합니다.
- 의도:
  - 실패한 문장도 로그 파일에 오류 정보로 남는지 확인합니다.
- 검사:
  - 종료코드 `1`
  - `sql_processor.log` 파일 존재
  - `ERROR INSERT users column count does not match value count` 문자열 포함

## 3) `logging_appends_across_multiple_runs`
- 시나리오:
  - 같은 디렉토리에서 프로그램을 두 번 실행합니다.
- 의도:
  - 로그 파일이 매 실행마다 append 되고 이전 기록을 덮어쓰지 않는지 확인합니다.
- 검사:
  - 종료코드 `0`
  - `sql_processor.log` 파일 존재
  - 로그 줄 수가 2줄 이상
  - 첫 실행과 두 번째 실행의 성공 기록이 모두 존재
