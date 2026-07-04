# Network Module

The network module defines transport metadata and the abstract `NetworkManager`
interface. It does not implement sockets, DDS participants, discovery,
serialization, or robot communication.

Network implementations must remain behind `NetworkManager` or adapter-specific
interfaces in downstream packages.
