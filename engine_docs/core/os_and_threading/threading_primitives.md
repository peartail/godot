# Threading Primitives

## Scope

Thread creation, mutexes, semaphores, condition variables, reader-writer locks, spin locks, and thread-safety helpers.

## Entry Points

- `Thread`
- `Mutex`
- `Semaphore`
- `ConditionVariable`
- `RWLock`
- `SpinLock`
- `SafeBinaryMutex`

## Flow Notes

- Threading primitives are lightweight wrappers around platform implementations.
- Lock ordering and object lifetime are caller responsibilities.
- Scene/object access from worker threads usually needs explicit synchronization or main-thread handoff.

## Code Links

- `core/os/thread.*`
- `core/os/mutex.*`
- `core/os/semaphore.h`
- `core/os/condition_variable.h`
- `core/os/rw_lock.h`
- `core/os/spin_lock.h`
- `core/os/safe_binary_mutex.h`
- `core/os/thread_safe.*`