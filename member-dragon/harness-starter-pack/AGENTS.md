# Agent Workspace Guide

이 하네스는 `mini-sql-parser`의 `member-dragon/` 작업을 안내합니다. 대화보다 저장소 문서를 우선하고, 구현과 검증의 흔적을 남깁니다.

## 먼저 읽을 문서

1. `config/project.harness.yaml`
2. `ARCHITECTURE.md`
3. `docs/design-docs/index.md`
4. `docs/design-docs/domain-rules.md`
5. `docs/product-specs/index.md`
6. `docs/exec-plans/active/current-plan.md`
7. `common/docs/io-spec.md`
8. `common/docs/error-messages.md`
9. `member-dragon/plan_codex.md`

## 기본 작업 규칙

- `common/`은 읽기 전용이다.
- 구현은 `member-dragon/src/` 안에서만 한다.
- 현재 범위는 `INSERT`와 `SELECT` 처리다.
- stderr 에러 문자열은 `common/docs/error-messages.md`의 exact string만 사용한다.
- 검증은 `common/scripts/run_tests.sh`로 한다.
- 주요 단계는 루프 로그에 남긴다.

## 출력 기본 계약

- 무엇을 읽었는지
- 무엇을 바꾸려는지
- 어떤 검증을 했는지
- 남은 리스크가 무엇인지

세부 규칙은 `config/project.harness.yaml`과 도메인 문서를 우선합니다.
