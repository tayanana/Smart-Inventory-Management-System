#include "InventoryService.h"
#include "Validation.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace {

int g_failed = 0;
int g_passed = 0;

void check(bool condition, const char* expression, const char* file, int line) {
    if (condition) {
        ++g_passed;
        return;
    }
    ++g_failed;
    std::cerr << "FAIL " << file << ":" << line << "  " << expression << '\n';
}

#define CHECK(cond) check((cond), #cond, __FILE__, __LINE__)

fs::path makeTempDir(const std::string& tag) {
    const fs::path dir =
        fs::temp_directory_path() / "autoinv_tests" /
        (tag + "-" + std::to_string(fs::file_time_type::clock::now().time_since_epoch().count()));
    fs::create_directories(dir);
    return dir;
}

void copyNamed(const fs::path& from, const fs::path& to) {
    fs::copy_file(from, to, fs::copy_options::overwrite_existing);
}

fs::path fixtureDir() {
    return fs::path(TEST_FIXTURE_DIR);
}

void testEmptyInventory() {
    const fs::path dir = makeTempDir("empty");
    InventoryService service(dir.string());
    const OperationResult loaded = service.load();
    CHECK(loaded.ok);
    CHECK(service.productCount() == 0);
    CHECK(service.supplierCount() == 0);
    CHECK(service.transactionCount() == 0);
    CHECK(service.loadWarnings().empty());
    CHECK(service.findProduct("ANYTHING") == nullptr);
    CHECK(service.productsSortedByStock().empty());
    CHECK(service.lowStock(5).empty());
}

void testInvalidInput() {
    const fs::path dir = makeTempDir("invalid");
    InventoryService service(dir.string());
    service.load();

    CHECK(!service.addProduct("", "Empty ID", 1, "").ok);
    CHECK(!service.addProduct("   ", "Whitespace ID", 1, "").ok);
    CHECK(!service.addProduct("WBAI2345678901234", "VIN with I", 1, "").ok);
    CHECK(!service.addProduct("WBAO2345678901234", "VIN with O", 1, "").ok);
    CHECK(!service.addProduct("WBAQ2345678901234", "VIN with Q", 1, "").ok);
    CHECK(!service.addProduct("PART1", "", 1, "").ok);
    CHECK(!service.addProduct("PART1", "Desc", -1, "").ok);
    CHECK(!service.addSupplier("", "Name", "x").ok);
    CHECK(!service.addSupplier("SUP1", "", "x").ok);
    CHECK(service.productCount() == 0);

    CHECK(validation::isValidVIN("1HGCM82633A004352"));
    CHECK(!validation::isValidVIN("SHORT"));
    std::string error;
    CHECK(validation::isValidProductId("M271_UCP_PIPE", error));
}

void testDuplicates() {
    const fs::path dir = makeTempDir("dup");
    InventoryService service(dir.string());
    service.load();

    CHECK(service.addSupplier("BOSCH", "Bosch", "a@b.c").ok);
    CHECK(!service.addSupplier("bosch", "Duplicate", "x").ok);
    CHECK(service.addProduct("PAD1", "Pad", 4, "BOSCH").ok);
    CHECK(!service.addProduct("pad1", "Same SKU", 1, "").ok);
    CHECK(service.recordTransaction("TX1", "2026-09-01", "PAD1", -1, "SALE").ok);
    CHECK(!service.recordTransaction("tx1", "2026-09-02", "PAD1", -1, "SALE").ok);
    CHECK(service.productCount() == 1);
    CHECK(service.supplierCount() == 1);
    CHECK(service.transactionCount() == 1);
}

void testNegativeQuantity() {
    const fs::path dir = makeTempDir("neg");
    InventoryService service(dir.string());
    service.load();
    CHECK(service.addProduct("FILTER", "Oil filter", 2, "").ok);
    CHECK(!service.recordTransaction("TX1", "2026-09-01", "FILTER", -3, "REPAIR").ok);
    CHECK(service.findProduct("FILTER")->getStockQuantity() == 2);
    CHECK(service.recordTransaction("TX2", "2026-09-01", "FILTER", -2, "REPAIR").ok);
    CHECK(service.findProduct("FILTER")->getStockQuantity() == 0);
    CHECK(!service.recordTransaction("TX3", "2026-09-01", "FILTER", -1, "SALE").ok);
    CHECK(service.findProduct("FILTER")->getStockQuantity() == 0);
}

void testMalformedCsv() {
    const fs::path dir = makeTempDir("badcsv");
    const fs::path fixtures = fixtureDir();
    copyNamed(fixtures / "malformed_suppliers.csv", dir / "suppliers.csv");
    copyNamed(fixtures / "malformed_inventory.csv", dir / "inventory.csv");
    copyNamed(fixtures / "malformed_transactions.csv", dir / "transactions.csv");

    InventoryService service(dir.string());
    const OperationResult loaded = service.load();
    CHECK(loaded.ok);
    CHECK(!service.loadWarnings().empty());

    CHECK(service.findSupplier("MANN") != nullptr);
    CHECK(service.findSupplier("BOSCH") != nullptr);
    CHECK(service.supplierCount() == 2);

    auto oil = service.findProduct("OIL_FILTER");
    auto pad = service.findProduct("GOOD_PAD");
    auto comma = service.findProduct("PAD_COMMA");
    CHECK(oil != nullptr);
    CHECK(pad != nullptr);
    CHECK(comma != nullptr);
    CHECK(service.findProduct("WBAI2345678901234") == nullptr);
    CHECK(service.findProduct("NEG_STOCK") == nullptr);
    CHECK(service.productCount() == 3);

    CHECK(oil != nullptr && oil->getStockQuantity() == 12);
    CHECK(oil != nullptr && oil->getSupplier() != nullptr);
    CHECK(comma != nullptr && comma->getDescription() == "Pad, ceramic");

    CHECK(service.transactionCount() == 2);  // TX001 and TX004; others skipped
}

void testPersistenceRoundTrip() {
    const fs::path dir = makeTempDir("roundtrip");
    {
        InventoryService service(dir.string());
        service.load();
        CHECK(service.addSupplier("MANN", "Mann", "c").ok);
        CHECK(service.addProduct("PIPE", "Coolant pipe", 5, "MANN").ok);
        CHECK(service.addProduct("1HGCM82633A004352", "Used Civic", 1, "").ok);
        CHECK(service.recordTransaction("TX9", "2026-09-18", "PIPE", -1, "REPAIR").ok);
        CHECK(service.save().ok);
    }
    {
        InventoryService reloaded(dir.string());
        CHECK(reloaded.load().ok);
        CHECK(reloaded.productCount() == 2);
        CHECK(reloaded.findProduct("PIPE")->getStockQuantity() == 4);
        CHECK(reloaded.findProduct("PIPE")->getSupplier() != nullptr);
        CHECK(reloaded.findProduct("PIPE")->getSupplier()->getSupplierId() == "MANN");
        CHECK(reloaded.findProduct("1HGCM82633A004352") != nullptr);
        CHECK(reloaded.transactionCount() == 1);
    }
}

void testSupplierLinkAndSort() {
    const fs::path dir = makeTempDir("linksort");
    InventoryService service(dir.string());
    service.load();
    CHECK(service.addSupplier("BOSCH", "Bosch", "x").ok);
    CHECK(service.addProduct("B", "High stock", 20, "").ok);
    CHECK(service.addProduct("A", "Low stock", 1, "").ok);
    CHECK(service.linkProductToSupplier("A", "BOSCH").ok);
    CHECK(service.findProduct("A")->getSupplier()->getSupplierId() == "BOSCH");
    CHECK(!service.linkProductToSupplier("A", "MISSING").ok);

    const auto sorted = service.productsSortedByStock();
    CHECK(sorted.size() == 2);
    CHECK(sorted[0]->getSkuVin() == "A");
    CHECK(sorted[1]->getSkuVin() == "B");
    CHECK(service.lowStock(1).size() == 1);
}

void testEditAndRemove() {
    const fs::path dir = makeTempDir("editrm");
    InventoryService service(dir.string());
    service.load();
    CHECK(service.addSupplier("MANN", "Mann", "x").ok);
    CHECK(service.addProduct("PAD", "Old desc", 3, "").ok);
    CHECK(service.editProduct("PAD", std::string("New desc"), 7, true, "MANN").ok);
    CHECK(service.findProduct("PAD")->getDescription() == "New desc");
    CHECK(service.findProduct("PAD")->getStockQuantity() == 7);
    CHECK(service.findProduct("PAD")->getSupplier()->getName() == "Mann");
    CHECK(service.recordTransaction("TX1", "2026-09-01", "PAD", -1, "SALE").ok);
    CHECK(service.removeProduct("PAD").ok);
    CHECK(service.findProduct("PAD") == nullptr);
    CHECK(service.transactionCount() == 1);
}

}  // namespace

int main() {
    testEmptyInventory();
    testInvalidInput();
    testDuplicates();
    testNegativeQuantity();
    testMalformedCsv();
    testPersistenceRoundTrip();
    testSupplierLinkAndSort();
    testEditAndRemove();

    std::cout << g_passed << " checks passed";
    if (g_failed != 0) {
        std::cout << ", " << g_failed << " failed";
    }
    std::cout << '\n';
    return g_failed == 0 ? 0 : 1;
}
