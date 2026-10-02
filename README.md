# Automotive Workshop Inventory System

Command-line stock book for a small automotive workshop and dealership. It keeps **vehicles** (17-character VIN) and **spare parts** (internal SKU), links them to **suppliers**, and records **stock transactions**. Data lives in three CSV files so a restart reloads the last saved session.

C++17, CMake 3.10 or newer. No third-party libraries.

## Client and server (in-process)

This is a **client-server CLI**, not a network program. There are no sockets and no HTTP.

- **Client:** the numbered menu in `src/main.cpp`. It reads keystrokes and prints results.
- **Server-side logic:** `InventoryService` (rules, search, sort, reports).
- **Server-side store:** `DataLayer` (maps in memory and CSV files).

## Roles

At start, choose an interaction type (1 or 2, or the words `user` / `admin`):

| Role | What you can do |
| --- | --- |
| **Workshop user** | Search, sort by stock, low-stock report, list products, list suppliers, then exit. Cannot change stock. |
| **Administrator** | Add / edit / remove products, record transactions, manage suppliers, plus the same retrieve operations. **Save and exit** rewrites the CSV files. |

There is no password. A workshop terminal is assumed to be physically shared; pick the role that matches the task.

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

Administrators quit with **11 (Save and exit)** so changes are written back to disk. Workshop users quit with **6 (Exit)** and do not rewrite files.

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
| `smoke_persist` | CLI: administrator adds a part → save/quit → workshop user restarts → search finds the same record |

You can also run the binaries directly:

```bash
./build/inventory_tests
bash tests/smoke_persist.sh ./build/AutoInventorySystem
```

Fixtures live in `tests/fixtures/`.

## Administrator menu

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
src/         implementations and the CLI client (`main.cpp`)
data/        sample CSV files
tests/       unit tests, fixtures, CLI smoke script
docs/        code book and CSV specification
```
