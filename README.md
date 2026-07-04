# res-project

A C project built with Make, set up to run inside a Dev Container.

## Layout

```
.
├── .devcontainer/
│   └── devcontainer.json   # Dev Container definition (Ubuntu + GCC + Make)
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
The container image ships with GCC and Make preinstalled.

## Building

```bash
make            # debug build (default) -> build/res-project
make BUILD=release
make run        # build and run
make clean      # remove build output
```
