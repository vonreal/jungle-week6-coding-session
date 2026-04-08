# Design Docs Index

이 폴더는 `mini-sql-parser`가 지켜야 하는 설계 제약을 적는다.

## 기본 문서

- `golden-principles.md`: 이 프로젝트 전반에 적용되는 핵심 원칙
- `domain-rules.md`: SQL 처리기 전용 규칙과 에러 우선순위
- `data-contracts.md`: 필요하면 데이터 구조 계약을 추가
- `compliance.md`: 보안이나 규제 요구가 생기면 추가

## 이 프로젝트에서의 역할

- 구현 범위와 금지 범위를 고정한다.
- 파서와 실행기의 책임 경계를 분리한다.
- 테스트가 기대하는 출력 규칙을 명시한다.
- `common/`과 `member-dragon/src/`의 역할 분리를 유지한다.
