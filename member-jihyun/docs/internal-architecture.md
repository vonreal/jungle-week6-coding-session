# Jihyun 구현 구조 메모

## 목표
과제의 외부 요구사항은 그대로 지키면서, 내부 구조는 아래처럼 나눈다.

`SQL 파일 -> Parser -> AST -> Executor -> Storage -> Page -> Buffer Cache -> 파일`

## 왜 이 구조를 쓰는가
- 과제 핵심 흐름인 `입력 -> 파싱 -> 실행 -> 저장`이 코드에서 그대로 보인다.
- 발표 때 “Mini SQL Processor”와 “Page/Buffer Cache 차별점”을 같이 설명하기 쉽다.
- 지금은 `INSERT`, `SELECT`만 구현하더라도 이후 `WHERE`, `DELETE` 같은 확장 포인트가 남는다.

## 파일 구성

### 엔트리
- [`main.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/main.c)
  - CLI 인자 확인
  - SQL 파일 읽기
  - statement 분리
  - parse / execute 루프
  - 종료 코드 결정

### 파싱 계층
- [`parser.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/parser.c)
  - 세미콜론 기준 SQL 문장 분리
  - `INSERT INTO ... VALUES ...`
  - `SELECT ... FROM ...`
  - AST 생성

- [`ast.h`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/include/ast.h)
  - `Statement`, `InsertStatement`, `SelectStatement` 정의

### 실행 계층
- [`executor.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/executor.c)
  - schema 로드
  - 컬럼 검증
  - insert/select 실행
  - 표준 에러 메시지 출력

### 메타데이터 계층
- [`schema.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/schema.c)
  - `<table>.schema` 읽기
  - 컬럼 목록 추출
  - 컬럼 index 조회

### 저장 계층
- [`storage.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/storage.c)
  - row 직렬화 / 역직렬화
  - 테이블별 `.data` 파일 접근
  - page append / scan 연결

### 페이지 계층
- [`page.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/page.c)
  - 4096 byte page 포맷
  - page header 관리
  - record append
  - page cursor 순회

### 캐시 계층
- [`buffer_cache.c`](/c:/Users/jihyun/ws/jungle-week6-coding-session/member-jihyun/src/buffer_cache.c)
  - `(file_path, page_id)` 기준 frame 캐싱
  - dirty page flush
  - 간단한 LRU 비슷한 교체

## 데이터 흐름

### INSERT
1. SQL 파일에서 statement를 읽는다.
2. Parser가 `INSERT` AST를 만든다.
3. Executor가 schema를 읽고 컬럼 수 / 컬럼명을 검증한다.
4. Storage가 row를 record로 직렬화한다.
5. Page에 record를 append 한다.
6. Buffer Cache가 dirty page를 관리하고 flush 한다.

### SELECT
1. Parser가 `SELECT` AST를 만든다.
2. Executor가 schema와 선택 컬럼을 검증한다.
3. Storage가 테이블 파일을 page 단위로 순회한다.
4. row를 복원해 선택 컬럼만 출력한다.
5. row가 하나도 없으면 아무 것도 출력하지 않는다.

## 구현 우선순위
1. `main + parser + executor`로 최소 실행 흐름을 만든다.
2. `schema + storage`로 공개 테스트를 먼저 통과시킨다.
3. `page + buffer_cache`를 얹어 차별점을 만든다.
4. 문서(`decisions`, `prompts`, `trouble`)를 작업과 함께 갱신한다.

## 이번 구조의 강점
- 과제 요구사항 충족에 직접적인 구조다.
- 파일 기반 저장이라는 조건을 만족한다.
- 내부적으로는 DBMS의 page/cache 개념을 맛볼 수 있다.
- 발표 때 “왜 이렇게 나눴는지” 설명하기 좋다.
