# Harness Starter Pack

이 스타터팩은 하네스 구조를 새 프로젝트에 빠르게 이식하기 위한 기본 골격입니다.

핵심 목표는 세 가지입니다.

- 에이전트 작업 흐름을 문서와 규칙으로 고정하기
- 자동 루프 진행 상황을 로그로 남기기
- 프로젝트마다 요구사항, 산출물 형식, 문서 구성을 유연하게 바꾸기

## 포함된 구성

- `AGENTS.md`: 에이전트가 가장 먼저 읽는 작업 규칙
- `ARCHITECTURE.md`: 상위 구조와 책임 분리
- `config/project.harness.yaml`: 프로젝트별 유연한 설정
- `docs/design-docs/*`: 설계 원칙과 구조 문서
- `docs/product-specs/*`: 요구사항과 기능 명세 문서
- `docs/exec-plans/active/current-plan.md`: 현재 실행 계획
- `docs/ops/logging.md`: 자동 루프 로그 운영 방식
- `runtime/loop-logs/example-session.jsonl`: 예시 로그
- `schemas/loop-event.schema.json`: 로그 이벤트 스키마
- `scripts/init_loop_log.py`: 세션 로그 초기화
- `scripts/append_loop_log.py`: 루프 이벤트 추가

## 빠른 시작

1. 이 폴더를 새 프로젝트 루트로 복사합니다.
2. `config/project.harness.yaml`에서 프로젝트 이름, 단계, 승인 정책, 로그 정책을 바꿉니다.
3. `docs/design-docs`와 `docs/product-specs`에 프로젝트 전용 규칙을 채웁니다.
4. 자동 루프나 에이전트 실행기에서 `scripts/init_loop_log.py`와 `scripts/append_loop_log.py`를 호출해 진행 로그를 남깁니다.

## 유연성 원칙

- 코어 작업 단계는 유지하되, 단계별 pause 정책은 프로젝트마다 바꿀 수 있습니다.
- 문서 필수 목록과 선택 문서는 `project.harness.yaml`에서 바꿀 수 있습니다.
- 결과 응답 형식의 필수 섹션과 선택 섹션도 설정으로 바꿀 수 있습니다.
- 도메인 규칙, 보안 규칙, 출시 체크리스트 같은 문서는 필요할 때만 추가할 수 있습니다.

## 로그 예시

```bash
python3 scripts/init_loop_log.py --project-name "My Project"
python3 scripts/append_loop_log.py \
  --file runtime/loop-logs/2026-04-08-session-001.jsonl \
  --event stage_completed \
  --stage plan \
  --status completed \
  --summary "Approved implementation plan"
```

자동 루프가 붙으면 위 명령을 직접 치지 않아도 되고, 하네스 코어가 같은 포맷으로 기록만 남기면 됩니다.
