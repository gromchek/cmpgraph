# cmpgraph

A command-line tool for comparing call graphs between different versions of a binary, designed to assist in identifying and matching functions across builds.

## Requirements

- **C++ compiler**: `g++` with C++ support
- **clang-format** (optional, for code formatting)
- **make**

## Building

```bash
make all
```

## Extract Data from Ghidra

1) Open the `Script Manager` (`Window` -> `Script Manager`).
2) Add `ExportCallgraph.py` to scripts and run it.
3) When the script finishes, save the output result as a file named `335_call_graph.json` in the project root directory.


## Usage

```bash
./cmpgraph --base <base_call_graph.json> --ref <ref_call_graph.json> -o <output_file.txt>
```

### Arguments

| Argument | Description |
|----------|-------------|
| `--base` | Path to the base call graph JSON (e.g., from version 3.3.5) |
| `--ref`  | Path to the reference call graph JSON (e.g., from a newer version) |
| `-o`     | Output file path for the comparison result |

## Predefined Run Targets

Several convenience targets are available for common version comparisons against the `335` base:

```bash
make run_410   # Compare 3.3.5 -> 4.1.0
make run_501   # Compare 3.3.5 -> 5.0.1
make run_601   # Compare 3.3.5 -> 6.0.1
make run_053   # Compare 3.3.5 -> 0.5.3
make run_all   # Run all of the above
```

Each target produces a corresponding `<version>result.txt` file.

## Post-Processing

After running comparisons, you can filter out noise and irrelevant entries using the `to_proc` target:

```bash
make to_proc
```

## Testing

Run the string transformer unit tests:

```bash
make test
```