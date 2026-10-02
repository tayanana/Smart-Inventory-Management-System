# CSV file format

Three UTF-8 text files in the data directory (current directory, or the path passed as the first command-line argument). A header row is written on save and skipped on load when the first column name matches the schema below.

Lines are comma-separated. A field that contains a comma, a double quote, or a newline is wrapped in double quotes; a quote inside a field is written as `""`. Extra columns on a data row are ignored. A row with too few columns is skipped and reported.

## `suppliers.csv`

| Column | Meaning |
| --- | --- |
| `supplier_id` | Unique vendor key (stored uppercased) |
| `name` | Display name |
| `contact` | Free-text contact |

Example:

```csv
supplier_id,name,contact
BOSCH,Bosch Automotive,parts@bosch.example
```

## `inventory.csv`

| Column | Meaning |
| --- | --- |
| `sku_vin` | Part SKU or 17-character VIN |
| `description` | Item text |
| `stock_quantity` | Non-negative integer; this is the **current** stock |
| `supplier_id` | Vendor key, or empty if unlinked |

Example:

```csv
sku_vin,description,stock_quantity,supplier_id
M271_UCP_PIPE,M271 under-chassis coolant pipe,4,BOSCH
1HGCM82633A004352,Used Honda Civic 1.8,1,
```

A 17-character `sku_vin` must be a plausible ISO 3779 VIN: letters and digits only, excluding **I**, **O**, and **Q**. Shorter part SKUs may use letters, digits, underscore, and hyphen.

## `transactions.csv`

| Column | Meaning |
| --- | --- |
| `transaction_id` | Unique movement id (stored uppercased) |
| `date` | Preferred `YYYY-MM-DD` |
| `sku_vin` | Product that moved |
| `quantity_change` | Signed integer (negative = consumption/sale) |
| `type` | `REPAIR`, `SALE`, `RESTOCK`, or other label |

Example:

```csv
transaction_id,date,sku_vin,quantity_change,type
TX001,2026-09-02,OIL_FILTER_F,-2,REPAIR
```

Quantity changes are **not** applied again at load time. Reloading uses `inventory.csv` for stock and this file only as history.

Load order: suppliers → inventory (resolve `supplier_id`) → transactions (resolve `sku_vin`).

## Missing and malformed files

| Situation | Behaviour |
| --- | --- |
| File does not exist | That collection starts empty. The program does not crash. |
| Empty file or header only | Empty collection. |
| Blank line | Ignored. |
| Too few columns, empty id, non-integer quantity, negative stock, invalid VIN, duplicate id | Line skipped; a warning is printed after load. Valid rows are kept. |
| Product names an unknown supplier | Product is stored with no vendor; warning printed. |
| Transaction names an unknown SKU | Transaction skipped; warning printed. |

Save overwrites all three files. Inventory and supplier rows are written sorted by ID; transactions stay in chronological order.
