# License Header Template

The repository license is Apache License 2.0. New source files should include a
short SPDX header where practical.

Do not rewrite existing files solely to add license headers. Add the header when
creating new files or when substantially modifying a file for other reasons.

## C++ Source and Header Files

```cpp
// SPDX-License-Identifier: Apache-2.0
```

## CMake Files

```cmake
# SPDX-License-Identifier: Apache-2.0
```

## Shell Scripts

```bash
# SPDX-License-Identifier: Apache-2.0
```

For generated files, preserve the generator's expected header format and include
SPDX metadata only when it does not interfere with tooling.
