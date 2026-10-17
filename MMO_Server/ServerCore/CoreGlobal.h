#pragma once

extern class ThreadManager* GThreadManager;

// 락 획득 순서를 기록해 데드락 가능성을 탐지하는 전역 프로파일러 (생성/소멸은 CoreGlobal.cpp)
extern class DeadLockProfiler* GDeadLockProfiler;
