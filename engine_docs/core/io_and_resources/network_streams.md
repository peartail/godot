# Network Streams

## Scope

Stream and packet peers, TCP/UDP/UDS sockets, TLS/DTLS wrappers, socket servers, and HTTP client support.

## Entry Points

- `StreamPeer`
- `StreamPeerTCP`
- `PacketPeer`
- `PacketPeerUDP`
- `TCPServer`
- `UDPServer`
- `HTTPClient`
- `StreamPeerTLS`
- `PacketPeerDTLS`

## Flow Notes

- Stream peers expose ordered byte streams while packet peers expose packet-oriented transport.
- Socket classes use platform net socket implementations behind `NetSocket`.
- TLS/DTLS availability depends on compiled crypto/TLS backend support.

## Code Links

- `core/io/stream_peer*`
- `core/io/packet_peer*`
- `core/io/tcp_server.*`
- `core/io/udp_server.*`
- `core/io/socket_server.*`
- `core/io/net_socket.*`
- `core/io/http_client*`
- `core/io/dtls_server.*`