# Customization Guide

프로젝트마다 요구사항과 문서 구조가 다르기 때문에, 코어 하네스와 프로젝트별 차이를 분리해야 합니다.

## 바꾸는 위치

- `config/project.harness.yaml`
  단계, 승인 정책, 로그 정책, 필수 문서, 출력 계약을 바꿉니다.
- `docs/design-docs/*`
  아키텍처 제약, 도메인 규칙, 보안/규제 요구를 적습니다.
- `docs/product-specs/*`
  기능 목표, 성공 조건, 테스트 기준을 적습니다.

## 유연하게 바꿀 수 있는 것

- `workflow.stages`
  예: `discover -> plan -> implement -> verify -> review -> release`
- `docs.custom_templates`
  예: `privacy.md`, `seo-guidelines.md`, `analytics-events.md`
- `outputs.optional_sections`
  예: `migration_notes`, `customer_impact`, `cost_estimate`

## 추천 방식

- 코어 루프는 유지합니다.
- 프로젝트 차이는 문서와 설정으로 풀어냅니다.
- 정말 필요한 경우에만 단계를 추가합니다.

이렇게 해야 여러 프로젝트에 같은 하네스를 안정적으로 재사용할 수 있습니다.
