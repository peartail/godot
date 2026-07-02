# Worker Thread Pool

## Scope

Shared worker task scheduling, task groups, low-priority tasks, and thread-pool lifecycle.

## Entry Points

- `WorkerThreadPool`
- `WorkerThreadPool::add_task()`
- `WorkerThreadPool::add_group_task()`
- `WorkerThreadPool::wait_for_task_completion()`
- `WorkerThreadPool::wait_for_group_task_completion()`

## Flow Notes

- The worker pool is a process-wide object service used by engine subsystems that need background work.
- Group tasks split repeated work across workers and track group completion.
- Callers must ensure captured data remains valid until the task completes.
- Work that touches scene objects usually needs main-thread handoff or explicit synchronization.

## Code Links

- `core/object/worker_thread_pool.h`
- `core/object/worker_thread_pool.cpp`
- `core/os/thread.*`
- `core/os/semaphore.h`