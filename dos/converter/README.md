# json2crane - JSON to Binary Converter

Converts Crane JSON project files to compact binary format for DOS.

## Building

```bash
make
```

## Usage

```bash
./json2crane input.json
```

This will create `input.dat` in the same directory.

## Format

The converter reads the JSON format used by the web version of Crane and outputs a binary dump of the `struct project` structure defined in `project.h`. This reduces file sizes by ~11x and eliminates the need for a JSON parser in the DOS version.

Example: `juno_room_current.json` (314KB) → `juno_room_current.dat` (28KB)

## Details

- Handles up to 100 tiles (MAX_TILES)
- 32×32 background grid
- Stores palette per tile instance (SNES-style), not per tile definition
- Empty background tiles are marked with -1
