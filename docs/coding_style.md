# Coding Style

humanoid-core uses modern C++17 with LLVM formatting.

Required practices:

- Use namespaces under `humanoid`.
- Use RAII for owned resources.
- Use `std::shared_ptr` or value ownership for injected interfaces.
- Use `enum class` for scoped enumerations.
- Use `std::chrono` for time and `std::filesystem` for paths.
- Document public headers, classes, and functions with Doxygen comments.
- Return `humanoid::common::Status` for recoverable framework operation results.

Prohibited practices:

- No singletons.
- No global variables.
- No raw owning pointers.
- No `using namespace std`.
- No vendor SDK includes in core modules.
- No macros for ordinary constants or control flow.
