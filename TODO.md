# TODO

- [ ] Refine command parsing in the shell (`Userlandsource/core/shell`): replace the if/else chain with a lookup table (command name -> handler) so new built-ins scale without touching the parser.
- [ ] Finish the `pkg` tool (`Userlandsource/core/pkg`): fetch tarball (see `//TODO` in `pkg.c`), install/remove, and index handling against the package server.
- [ ] Finish the userland in C (`Userlandsource`) for better compatibility and performance: complete the C++ -> C port, remove leftover C++ code/build rules, and make sure everything builds in `BuildDistro`.
