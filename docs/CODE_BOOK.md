# Code book

This file is the map of the program: where things live, who owns memory, and how to test it.

## Layers

| Layer | Files | Responsibility |
| --- | --- | --- |
| Interface | `src/main.cpp` | Numbered menu, prompts, printing. No CSV and no stock rules. |
| Logic | `include/InventoryService.h`, `src/InventoryService.cpp` | Add/edit/remove, supplier link, transactions, search, sort, low-stock report, VIN/SKU checks. |
| Data | `include/{Product,Supplier,Transaction,DataLayer}.h`, `src/DataLayer.cpp` | In-memory maps/vector and CSV load/save. |
| Shared validation | `include/Validation.h`, `src/Validation.cpp` | Trim, ID normalisation, ISO 3779 VIN, integer parse. |

The CLI constructs one `InventoryService` and talks only to that type.

## Types

| Type | Role | Identity |
| --- | --- | --- |
| `Supplier` | Vendor | `supplier_id` |
| `Product` | Vehicle (VIN) or part (SKU) | `sku_vin` |
| `Transaction` | Stock movement | `transaction_id` |
| `DataLayer` | Store + files | — |
| `InventoryService` | Operations | — |
| `OperationResult` | `{ok, message}` from logic | — |

Relationships: many products to one supplier; many transactions to one product.

## Memory and pointers

- No manual `new` / `delete`. Objects are created with `std::make_shared` and live in RAII containers.
- `DataLayer` holds `std::unordered_map<std::string, std::shared_ptr<Supplier>>` and `std::unordered_map<std::string, std::shared_ptr<Product>>` for average O(1) lookup by ID.
- History is `std::vector<Transaction>` in chronological order. Each `Transaction` stores `std::shared_ptr<Product>` so a later **remove** from the active map does not leave a dangling pointer; the product object stays alive while any transaction still refers to it.
- Removing a product drops it from `inventory.csv` on the next save. Transaction rows that name a SKU no longer in inventory are skipped on the next load (active stock file is the source of truth after restart).

## Algorithms

- **Search:** hash-map lookup by normalised (trimmed, uppercased) SKU/VIN.
- **Sort:** copy product pointers into a `std::vector`, sort by `stock_quantity` ascending, then by ID.
- **Low-stock report:** same order, keep items with `stock_quantity <= threshold`.
- **Transactions:** refuse a change when `current + delta < 0`; otherwise adjust stock and append history. History is **not** replayed on load — `inventory.csv` already stores the current quantity.
- **CSV:** full rewrite of all three files on graceful exit (menu 11, or EOF on the menu). Fields with commas or quotes are quoted; `""` escapes a quote.

## File map

| Path | Notes |
| --- | --- |
| `CMakeLists.txt` | C++17 executable `AutoInventorySystem` and test target `inventory_tests` |
| `include/` | Headers; CMake include path is this lowercase directory |
| `src/main.cpp` | CLI only |
| `src/DataLayer.cpp` | `loadFromCSV` / `saveToCSV` / add / remove |
| `src/InventoryService.cpp` | Business rules |
| `data/*.csv` | Sample workshop snapshot |
| `tests/test_inventory.cpp` | Assertion-style unit tests (no GoogleTest) |
| `tests/fixtures/` | Malformed CSV cases |
| `tests/smoke_persist.sh` | Add → quit → restart → search |

## How to run tests

From the repository root, after a successful CMake build:

```bash
cd build && ctest --output-on-failure
```

`inventory_tests` writes temporary directories under the system temp folder and must not be pointed at `data/` (it would not isolate cases). `smoke_persist` launches the real CLI with piped input.
