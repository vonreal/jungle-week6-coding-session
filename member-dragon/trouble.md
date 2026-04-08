# 삽질 로그

## 이슈 1: 첫 공개 테스트에서 SELECT가 세그폴트 남
- **상황**: 빌드는 성공했는데 `01_basic` 공개 테스트에서 첫 `SELECT` 실행 시 segmentation fault가 발생했다.
- **원인**: `executor.c`에서 `ResultSet`을 스택에 직접 잡았는데, 고정 크기 버퍼가 커서 macOS 스택 한계를 넘겼다.
- **AI가 준 답**: `ResultSet` 메모리 소유권을 명확히 하고, 큰 결과 버퍼는 스택 대신 힙으로 옮기자고 제안했다.
- **실제 해결**: `execute_command()`에서 `ResultSet`을 `malloc`으로 생성하고 사용 후 `free`하도록 바꿨다.
- **배운 것**: 고정 크기 구조체는 단순하지만, 크기가 커지면 스택/힙 배치를 먼저 따져야 한다.

## 이슈 2: hidden `08_mixed_errors`에서 에러 문자열이 하나 틀림
- **상황**: 숨은 테스트는 거의 다 통과했지만, `INSERT INTO users (name, age) ...` 케이스에서 `ERROR: invalid query`가 나와 실패했다.
- **원인**: INSERT 컬럼 검증을 너무 먼저 강하게 적용하면서, 이 프로젝트가 기대하는 `column count does not match value count` 우선순위를 덮어버렸다.
- **AI가 준 답**: 컬럼 목록을 계속 검증하되, 개수 불일치는 column mismatch로, 같은 개수의 이름/순서 오류만 invalid query로 분리하자고 정리했다.
- **실제 해결**: `validate_insert_shape()`의 반환 우선순위를 수정해 count mismatch는 `ERR_COLUMN_MISMATCH`로 매핑했다.
- **배운 것**: SQL 엔진의 정확도만큼 중요한 게 테스트가 기대하는 에러 우선순위다.

## 이슈 3: 과제 요구사항은 파일 저장인데 초기 플랜이 메모리 저장으로 기울어 있었음
- **상황**: 공통 테스트는 통과했지만, 원 과제 설명을 다시 확인해 보니 `INSERT`는 파일 저장, `SELECT`는 파일 읽기를 요구하고 있었다.
- **원인**: 테스트 통과 중심으로 생각하면서 저장 계층을 메모리로 단순화한 초안이 그대로 구현 방향에 남아 있었다.
- **AI가 준 답**: `StorageOps` vtable은 유지한 채 backend를 `file_storage.c`로 교체하고, 각 테이블을 `<table>.data` 파일로 관리하도록 수정하자고 제안했다.
- **실제 해결**: 메모리 저장소를 제거하고 file-backed 저장소를 구현해 `INSERT`는 `.data` 파일 append, `SELECT`는 `.data` 파일 read로 바꿨다.
- **배운 것**: 테스트를 통과해도 원 요구사항과 어긋날 수 있으니, 마지막엔 반드시 과제 본문과 구현을 다시 대조해야 한다.
