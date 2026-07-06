# Regression Test Guide

## Purpose

Regression testing compares a new build against a known accepted release.

## Regression Selection

Select cases based on changed areas:

- adapter or SDK changes: connection, telemetry, motion, hands, audio,
  reconnect, and fault injection,
- command or safety changes: command, emergency stop, capability, and battery
  validation,
- mission, runtime, behavior-tree, or planner changes: execution lifecycle and
  cancellation validation,
- perception changes: sensor, detection, and fusion validation,
- ROS2 or cloud changes: bridge and deployment validation.

## Reporting

Use `docs/Regression_Test_Report.md` or
`validation/reports/Regression_Report_Template.md`. Any change from `PASS` to
`FAIL`, `UNKNOWN`, or unplanned `SKIPPED` is a regression until triaged.
