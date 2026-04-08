# Feature Spec: Mini SQL Processor Core

## Goal

- `.sql` 파일의 `INSERT`와 `SELECT`를 실행한다.
- 현재 디렉토리의 `.schema` 파일을 읽어 `users`, `products` 테이블을 인식한다.
- `INSERT` 결과를 `<table>.data` 파일에 저장하고, `SELECT`는 그 파일에서 데이터를 읽는다.
- exact string 오류 규약으로 공개/히든 테스트를 안정적으로 통과한다.

## Success Criteria

- [x] `INSERT` 성공 시 stdout/stderr에 아무것도 남기지 않는다.
- [x] `INSERT`가 현재 디렉토리의 데이터 파일에 row를 추가한다.
- [x] `SELECT`는 헤더와 행을 `,`로 구분해 stdout에 출력한다.
- [x] `SELECT`는 현재 디렉토리의 데이터 파일에서 row를 읽는다.
- [x] 빈 줄은 무시한다.
- [x] quoted value 안의 공백과 콤마를 보존한다.
- [x] 잘못된 테이블, 잘못된 컬럼, 컬럼 수 불일치를 정확한 에러 문자열로 처리한다.
- [x] 에러가 난 뒤에도 다음 SQL 문을 계속 실행한다.

## Constraints

- 구현은 `member-dragon/src/`에만 둔다.
- `common/`은 수정하지 않는다.
- DB, 네트워크, 외부 저장소는 구현하지 않는다.
- 출력 문자열은 `common/docs/error-messages.md`와 일치해야 한다.

## Validation

- `./common/scripts/run_tests.sh ./member-dragon/src/sql_processor public`
- `./common/scripts/run_tests.sh ./member-dragon/src/sql_processor hidden`
- 실행 결과의 stdout/stderr를 기대 파일과 비교한다.

## Risks

- 고정 최대 크기(`MAX_ROWS`, `MAX_COLUMNS`)를 넘는 입력은 현재 범위 밖이다.
- JSON 스키마 파서는 현재 프로젝트 구조를 전제로 한 경량 구현이다.
- 데이터 파일 포맷은 length-prefixed custom format이라 사람이 바로 읽기엔 CSV보다 덜 직관적이다.
- 단일 따옴표 이스케이프 같은 확장 SQL 문법은 아직 지원하지 않는다.
