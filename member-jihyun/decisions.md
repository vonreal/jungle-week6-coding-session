# 설계 판단 기록

## 저장 포맷
- **결정**: 테이블별 `.data` 바이너리 파일 + 고정 크기 `Page(4096 bytes)` 구조를 사용한다.
- **이유**: 이번 과제의 최소 요구사항은 파일 기반 DB이지만, 내부 구조를 DBMS처럼 가져가고 싶어서 `Page` 단위로 저장 계층을 나눴다.
- **구체 방식**: 각 Page는 `page_id`, `next_page_id`, `row_count`, `used_bytes` 헤더를 가지며, payload에는 길이 기반 record를 순차 저장한다.
- **트레이드오프**: CSV보다 디버깅은 불편하지만, 값 안에 쉼표가 있어도 안정적으로 저장할 수 있고 Buffer Cache를 붙이기 쉽다.

## 파서 구조
- **결정**: `split -> parse -> AST` 구조로 분리한다.
- **이유**: SQL 파일 안에 여러 문장이 들어오므로 먼저 statement 분리기가 필요하고, 이후 `INSERT`와 `SELECT`를 AST로 변환하면 실행기와 결합이 느슨해진다.
- **구체 방식**:
  - `split_sql_statements`: 세미콜론 기준 문장 분리
  - `parse_statement`: 문장별 파싱
  - `Statement`: `INSERT`, `SELECT` 공용 AST
- **트레이드오프**: 현재 문법 범위가 작아서 단일 함수로도 가능하지만, 확장성은 분리형이 더 좋다.

## 실행 구조
- **결정**: `main -> parser -> executor -> storage -> page/buffer_cache` 계층을 사용한다.
- **이유**: 과제의 핵심 흐름인 `입력 -> 파싱 -> 실행 -> 저장`을 코드 구조에서 그대로 읽히게 만들기 위해서다.
- **트레이드오프**: 파일 수는 늘어나지만, 발표 때 설명하기 쉽고 기능별 책임이 명확해진다.

## 에러 처리 범위
- **결정**: 공개/히든 테스트 기준으로 아래 에러를 우선 정확히 맞춘다.
- **대상**:
  - `ERROR: invalid query`
  - `ERROR: table not found`
  - `ERROR: column count does not match value count`
  - `ERROR: file open failed`
- **이유**: 표준 문구를 문서와 동일하게 맞추는 것이 테스트 통과에 직접 연결된다.
- **포기한 것**: 타입 검증, 정교한 SQL 문법 오류 분류, 복잡한 복구 로직은 이번 범위에서 제외한다.

## INSERT 정책
- **결정**: 현재는 schema에 정의된 전체 컬럼이 모두 들어와야 성공으로 처리한다.
- **이유**: 과제 요구사항과 테스트 범위가 단순하며, 기본값/nullable 개념이 없다.
- **세부 처리**:
  - 컬럼 수와 값 수가 다르면 `column count does not match value count`
  - schema 컬럼 수와 다르게 일부 컬럼만 넣는 경우도 같은 에러로 처리
  - 컬럼 순서는 달라도 되지만, 내부적으로 schema 순서로 재정렬해 저장

## SELECT 정책
- **결정**: `SELECT *` 와 `SELECT col1, col2`만 지원한다.
- **이유**: 현재 공통 요구사항이 최소 `SELECT` 지원이며, 테스트도 이 범위에 맞춰져 있다.
- **세부 처리**:
  - 결과 row가 하나도 없으면 header도 출력하지 않는다
  - 존재하지 않는 컬럼을 조회하면 `invalid query`

## WHERE절 지원 여부
- **결정**: 미지원
- **이유**: 공통 요구사항에 없고, 현재 테스트 범위도 아니다.

## Buffer Cache
- **결정**: 소형 LRU 비슷한 정책의 간단한 Buffer Cache를 둔다.
- **이유**: 차별점 요소를 넣되, 구현 복잡도는 통제하기 위해서다.
- **구체 방식**:
  - `(table file path, page_id)` 기준으로 frame 관리
  - dirty page는 flush 시 파일에 반영
  - insert 후 해당 테이블만 flush 해서 파일 기반 동작과 테스트 일관성을 보장

## 파일 책임 분리
- [`main.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/main.c): CLI, SQL 파일 읽기, 전체 흐름 제어
- [`parser.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/parser.c): SQL 파싱과 AST 생성
- [`executor.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/executor.c): AST 실행
- [`schema.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/schema.c): `.schema` 로딩
- [`storage.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/storage.c): row 직렬화, 테이블 파일 접근
- [`page.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/page.c): Page 포맷과 record 순회
- [`buffer_cache.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/buffer_cache.c): 캐시 frame 관리

## 검증 기반 운영 결정(체크리스트 진행 중)
- **결정**: 기능 검증은 체크리스트 항목 단위로 분리 실행하고, 각 항목 성공 시 즉시 문서에 체크한다.
- **이유**: 구현 완료 여부를 주관적으로 판단하지 않고, 실행 결과 중심으로 추적하기 위해서다.
- **구체 방식**:
  - 1~7번 항목을 실제 명령 실행으로 검증
  - 실패 시 즉시 원인 분리(환경/스키마/문법) 후 재검증
  - 결과는 `project-checklist.md`에 즉시 반영
- **트레이드오프**: 실행 횟수는 늘어나지만, 제출 직전 리스크를 크게 줄일 수 있다.
