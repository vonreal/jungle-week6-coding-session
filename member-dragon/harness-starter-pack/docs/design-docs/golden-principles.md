# Golden Principles

- `common/`은 읽기 전용으로 유지한다.
- 구현은 `member-dragon/src/` 안에만 둔다.
- 현재 범위를 넘는 기능 확장은 하지 않는다.
- stderr 에러 문자열은 exact string으로 유지한다.
- `INSERT`와 `SELECT`의 공개 테스트를 먼저 만족시킨다.
- 검증은 `common/scripts/run_tests.sh`로 재현 가능해야 한다.
- 주요 단계와 실패는 루프 로그에 남긴다.

원칙은 짧고 실행 가능해야 한다. 설명보다 제약이 먼저 보이도록 유지한다.
