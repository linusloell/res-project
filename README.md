# res-project

A C project built with Make, set up to run inside a Dev Container.

## Layout

```
.
├── .devcontainer/
│   ├── Dockerfile          # Container image (Ubuntu + GCC + Make + tooling)
│   └── devcontainer.json   # Dev Container definition
├── include/                # Public headers
│   └── app.h
├── src/                    # Source files
│   ├── app.c
│   └── main.c
├── build/                  # Build output (git-ignored)
└── Makefile
```

## Dev Container

Open the folder in VS Code / Cursor and choose **Reopen in Container**
(or run *Dev Containers: Reopen in Container* from the command palette).
The image is built from `.devcontainer/Dockerfile` and ships with GCC, Make,
GDB, Valgrind, cppcheck, and clang-tidy. Add more packages to the Dockerfile
(e.g. a cross-compiler toolchain) as the project grows.

## Building

```bash
make            # debug build (default) -> build/res-project
make BUILD=release
make run        # build and run
make clean      # remove build output
```
