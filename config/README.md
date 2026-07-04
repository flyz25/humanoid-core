# Configuration

This directory stores configuration examples and schema documentation. The core
configuration module exposes interfaces only and does not implement a general
YAML parser.

`robot.yaml` is used by the factory-based robot connection example:

```yaml
robot:
  vendor: Unitree
  model: G1
  ip: 192.168.123.161
  network_interface: eth0
  timeout_ms: 500
  domain_id: 0
  serial_number:
  firmware:
```
