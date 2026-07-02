# Local And Remote Debugger

## Scope

Local debugger commands, remote debugger implementation, remote peer transport, and debugger marshaling.

## Entry Points

- `LocalDebugger`
- `RemoteDebugger`
- `RemoteDebuggerPeer`
- `DebuggerMarshalls`

## Flow Notes

- Local debugger supports command-line/debug builds without editor transport.
- Remote debugger talks to external editor/debugger clients through peer abstractions.
- Marshaling helpers encode debugger messages and data payloads.

## Code Links

- `core/debugger/local_debugger.*`
- `core/debugger/remote_debugger.*`
- `core/debugger/remote_debugger_peer.*`
- `core/debugger/debugger_marshalls.*`