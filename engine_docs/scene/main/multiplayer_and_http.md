# Multiplayer And HTTP

## Scope

Scene-level multiplayer API, multiplayer peers, and HTTP request node behavior.

## Entry Points

- `MultiplayerAPI`
- `MultiplayerPeer`
- `HTTPRequest`

## Flow Notes

- `MultiplayerAPI` integrates network replication/RPC behavior with scene tree nodes.
- Multiplayer peers abstract transport-specific network behavior.
- `HTTPRequest` wraps HTTP client behavior in a Node lifecycle.

## Code Links

- `scene/main/multiplayer_api.*`
- `scene/main/multiplayer_peer.*`
- `scene/main/http_request.*`
- `core/io/http_client*`