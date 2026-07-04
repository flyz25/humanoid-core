# Naming Convention

Namespaces use lower-case module names under `humanoid`, such as
`humanoid::robot` and `humanoid::logging`.

C++ naming:

- Classes and structs: `PascalCase`.
- Interfaces: descriptive `PascalCase`; external injection interfaces may use
  an `I` prefix when it clarifies adapter boundaries, such as `IRobotAdapter`.
- Functions and methods: `camelCase`.
- Enum values: `kPascalCase`.
- Private data members: trailing underscore, such as `controller_`.
- CMake targets: `humanoid_core_<module>` with installed aliases
  `humanoid::<module>`.

Files are named after the primary class or type they define.
