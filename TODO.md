# TODO

- [ ] Refine command parsing in the shell (`Userlandsource/core/shell`): replace the if/else chain with a lookup table (command name -> handler) so new built-ins scale without touching the parser.
- [ ] Finish the `pkg` tool (`Userlandsource/core/pkg`): fetch tarball (see `//TODO` in `pkg.cpp`), install/remove, and index handling against the package server.
