# Unitree Plugins

This directory contains Unitree-specific plugin packages and SDK boundaries for
humanoid-core. Plugins in this tree must keep vendor SDK headers and transport
details out of core framework interfaces.

Milestone 4.4 adds only the Unitree G1 plugin skeleton. It does not communicate
with Unitree SDK2, command physical hardware, or implement robot motion.

Milestone 4.5 adds `plugins/unitree/sdk`, the only production boundary allowed
to include Unitree SDK2 headers.
