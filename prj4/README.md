# System Programming project#4 - MyMallocator

**과목**: CSE4100 시스템 프로그래밍  
**학번**: 20210428  
**이름**: 정현정   

---

## 프로젝트 개요

MyMallocator는 C 프로그래밍에서 동적 메모리를 관리하는 **malloc**, **free**, **realloc** 함수들을 직접 구현하는 프로젝트입니다. 본 프로젝트는 기존 라이브러리 함수 대신 메모리 할당을 관리하는 **명시적 연결 리스트(explicit list)** 방식을 사용하여 효율적인 동적 메모리 할당기를 개발하는 것을 목표로 합니다.

### 주요 기능

MyMallocator는 다음과 같은 기능을 제공합니다:
1. **메모리 할당 (malloc)**: 요청된 크기의 메모리 block을 찾아 할당
2. **메모리 해제 (free)**: 지정된 메모리 block을 해제하고, 인접한 block이 비어 있는 경우 병합
3. **메모리 재할당 (realloc)**: 기존 메모리 block의 크기를 조정하여 새로운 크기의 메모리 block을 제공

---

## 프로젝트 구성

### 구현 함수 설명

- **mm_init()**: 초기 힙 영역을 설정하며, 프롤로그와 에필로그 block을 생성하고 초기 힙 확장을 수행
- **mm_malloc(size_t size)**: 요청된 `size` 크기에 맞는 block을 찾아 할당하며, 필요한 경우 힙을 확장
- **mm_free(void *ptr)**: 주어진 포인터의 메모리 블록을 해제하고, 인접한 free block이 있을 경우 병합하여 크기가 큰 free block을 생성
- **mm_realloc(void *ptr, size_t size)**: 주어진 block의 크기를 조정하여 새로운 크기의 메모리 block을 제공. 필요 시 힙을 확장하거나 병합을 수행

### 추가 구현된 서브루틴

- **extend_heap(size_t words)**: 힙을 지정된 워드 수만큼 확장하고, 새로 할당된 block을 free list에 추가하여 관리
- **find_fit(size_t asize)**: 할당 가능한 적절한 크기의 free block을 찾음
- **place(void *bp, size_t asize)**: block을 할당하고, 남는 공간이 충분할 경우 분할하여 나머지 공간을 free list에 추가
- **coalesce(void *bp)**: 해제된 block을 병합하여 자유 free block으로 만듦
- **add_free_list(size_t size, void *bp)** 및 **delete_free_list(void *bp)**: 자유 리스트의 삽입과 삭제 작업을 수행하여 탐색 효율성을 유지

---

## 주요 매크로 및 변수

- **NEXT_FREE(bp)**: 다음 자유 블록의 포인터 반환
- **PREV_FREE(bp)**: 이전 자유 블록의 포인터 반환
- **free_listp**: 자유 리스트의 첫 블록을 가리키는 포인터로, 자유 블록을 관리하는 기본 포인터

---

## 개발 환경

- **언어**: C
- **개발 및 실행 환경**: Linux