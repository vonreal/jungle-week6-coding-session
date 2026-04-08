# Loop Logging Guide

자동 루프가 도는 동안 "무슨 일이 있었는지"를 나중에 재구성할 수 있어야 합니다. 그래서 이 스타터팩은 로그를 단순 텍스트가 아니라 JSONL 이벤트 스트림으로 남기도록 권장합니다.

## 왜 JSONL인가

- 한 줄이 한 이벤트라 append가 쉽다
- 세션 재생과 필터링이 쉽다
- 사람이 읽기도 어렵지 않고, 스크립트로 후처리하기도 쉽다

## 권장 이벤트

- `session_started`
- `turn_started`
- `context_gathered`
- `stage_completed`
- `approval_requested`
- `approval_received`
- `validation_completed`
- `review_completed`
- `turn_completed`
- `turn_failed`

## 권장 필드

- `timestamp`
- `session_id`
- `turn_id`
- `event`
- `stage`
- `status`
- `summary`
- `artifacts`
- `changed_files`
- `tests`
- `risks`
- `next_step`
- `approval`

## 운영 팁

- 단계가 끝날 때마다 한 줄을 남깁니다.
- 승인 대기 지점은 별도 이벤트로 남깁니다.
- 실패도 성공만큼 중요하므로 `turn_failed`를 생략하지 않습니다.
- 긴 작업은 한 파일에 여러 턴이 들어가도 괜찮지만, 세션 ID는 유지합니다.
