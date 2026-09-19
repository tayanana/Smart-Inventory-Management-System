# Automotive Workshop Inventory System

Command-line stock book for a small automotive workshop and dealership. It keeps **vehicles** (17-character VIN) and **spare parts** (internal SKU), links them to **suppliers**, and records **stock transactions**. Data lives in three CSV files so a restart reloads the last saved session.

C++17, CMake 3.10 or newer. No third-party libraries.

## Build and run

From the repository root:

```bash
cmake -S . -B build
cmake --build build
./build/AutoInventorySystem
```

CSVs are read and written in the **current working directory** (`inventory.csv`, `suppliers.csv`, `transactions.csv`). Missing files start as an empty store.

To use the sample workshop data shipped in `data/`:

```bash
./build/AutoInventorySystem data
```

The optional first argument is the data directory.

If `cmake` fails because the default `c++` driver cannot link `libstdc++` (some Linux images use Clang as `/usr/bin/c++` without that library), configure with GCC explicitly:

```bash
cmake -S . -B build -DCMAKE_CXX_COMPILER=g++
cmake --build build
```

On macOS with Xcode Clang, the first pair of commands is enough.

Quit with menu option **11 (Save and exit)** so changes are written back to disk.

## Tests

```bash
cmake -S . -B build
cmake --build build
cd build && ctest --output-on-failure
```

This runs:

| Test | What it covers |
| --- | --- |
| `inventory_tests` | Empty inventory, invalid VIN/SKU, duplicates, negative quantity, malformed CSV, persistence round-trip, supplier link, sort, edit/remove |
| `smoke_persist` | CLI: add a part → save/quit → restart → search finds the same record |

You can also run the binaries directly:

```bash
./build/inventory_tests
bash tests/smoke_persist.sh ./build/AutoInventorySystem
```

Fixtures live in `tests/fixtures/`.

## Menu

1. Add product (vehicle or part)  
2. Edit product  
3. Remove product  
4. Record transaction (update stock)  
5. Search inventory by SKU/VIN  
6. Sort products by stock (lowest first)  
7. Low-stock report (you enter the threshold; **5** is a typical workshop value)  
8. Add supplier  
9. List suppliers  
10. Link product to supplier  
11. Save and exit  

Search is by ID only. Stock must never go below zero. A 17-character ID is treated as a VIN and must follow ISO 3779 (no letters I, O, or Q). Duplicate SKU/VIN and duplicate supplier IDs are rejected.

## Documentation

- [docs/CODE_BOOK.md](docs/CODE_BOOK.md) — files, types, ownership, layers  
- [docs/CSV_FORMAT.md](docs/CSV_FORMAT.md) — on-disk schema and malformed-file behaviour  

## Layout

```
include/     public headers (data types, store, logic)
src/         implementations and the CLI (`main.cpp`)
data/        sample CSV files
tests/       unit tests, fixtures, CLI smoke script
docs/        code book and CSV specification
```
