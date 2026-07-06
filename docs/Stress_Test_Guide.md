# Stress Test Guide

## Purpose

Stress tests observe resource stability and runtime behavior over extended
hardware operation. They do not replace safety supervision.

## Durations

Run durations are selected by the product profile:

- 4 hours for initial smoke endurance.
- 8 hours for daily operation validation.
- 12 hours for extended lab validation.
- 24 hours for release acceptance when required.

## Metrics

Collect:

- CPU usage,
- RAM usage,
- process handle/thread count,
- disk usage,
- battery state,
- motor and system thermals,
- network latency and packet loss,
- command latency,
- mission repetition count,
- telemetry update rate.

## Failure Conditions

Record `FAIL` for:

- process crash,
- deadlock,
- unbounded memory growth,
- repeated reconnect without recovery,
- command timeout burst,
- emergency-stop event,
- unsafe motion,
- thermal or battery limit violation.
