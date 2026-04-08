# Active Execution Plan

## Task

`member-dragon/src/`에 C 기반 mini SQL processor를 구현하고, 하네스 문서/로그까지 함께 남긴다.

## Scope

- 바꿀 것: `member-dragon/src/`의 SQL 처리기 구현, `member-dragon/plan_codex.md` 보정, 하네스 문서와 실행 로그, 개인 기록 문서
- 바꾸지 않을 것: `common/` 아래 테스트/스키마/스크립트, 범위 밖 SQL 기능(`WHERE`, `UPDATE`, `DELETE`, `JOIN`)

## Stages

1. Discover
2. Plan
3. Implement
4. Verify
5. Review

## Current Status

- Current stage: `review`
- Approval required: `false`
- Next step: `Review residual risks, sync root docs, and summarize the completed harness run`

## Artifacts

- spec: `member-dragon/plan_codex.md`, `member-dragon/harness-starter-pack/docs/product-specs/feature-template.md`
- design: `member-dragon/harness-starter-pack/ARCHITECTURE.md`, `member-dragon/harness-starter-pack/docs/design-docs/domain-rules.md`
- test output: public 6/6 PASS, hidden 8/8 PASS via `common/scripts/run_tests.sh`
- review notes: stack overflow risk in `SELECT` result handling fixed by moving `ResultSet` off the stack; INSERT error priority aligned to hidden test expectations; storage backend corrected from memory to file-backed `.data` tables to match the assignment brief

## Residual Risks

- 고정 크기 버퍼 기반이라 아주 큰 입력은 현재 범위 밖이다.
- 스키마 JSON 파서는 프로젝트의 단순한 `.schema` 포맷에 맞춘 경량 구현이다.
- 데이터 파일 포맷은 프로젝트 전용 custom encoding이다.
- SQL 확장은 `INSERT`/`SELECT` 외에는 아직 지원하지 않는다.
