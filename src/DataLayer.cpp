#include "DataLayer.h"
#include "Validation.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace {

std::string lowerCopy(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

std::string lineLabel(const std::string& file, int lineNumber) {
    std::ostringstream out;
    out << file << " line " << lineNumber;
    return out.str();
}

}  // namespace

DataLayer::DataLayer(std::string dataDirectory) : dataDir_(std::move(dataDirectory)) {
    if (dataDir_.empty()) {
        dataDir_ = ".";
    }
    while (dataDir_.size() > 1 && (dataDir_.back() == '/' || dataDir_.back() == '\\')) {
        dataDir_.pop_back();
    }
}

std::string DataLayer::filePath(const std::string& name) const {
    return (std::filesystem::path(dataDir_) / name).string();
}

std::vector<std::string> DataLayer::splitCSVLine(const std::string& line) const {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current.push_back(c);
            }
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            fields.push_back(validation::trim(current));
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    fields.push_back(validation::trim(current));
    return fields;
}

std::string DataLayer::escapeCSV(const std::string& field) {
    const bool quote = field.find_first_of(",\"\n\r") != std::string::npos;
    if (!quote) {
        return field;
    }
    std::string out;
    out.push_back('"');
    for (char c : field) {
        if (c == '"') {
            out += "\"\"";
        } else {
            out.push_back(c);
        }
    }
    out.push_back('"');
    return out;
}

bool DataLayer::isHeaderRow(const std::vector<std::string>& fields, const std::string& firstColumn) {
    if (fields.empty()) {
        return false;
    }
    return lowerCopy(fields[0]) == lowerCopy(firstColumn);
}

void DataLayer::loadFromCSV() {
    warnings_.clear();
    suppliers_.clear();
    inventory_.clear();
    transactions_.clear();
    loadSuppliers();
    loadInventory();
    loadTransactions();
}

void DataLayer::loadSuppliers() {
    const std::string name = "suppliers.csv";
    const std::string path = filePath(name);
    std::ifstream in(path);
    if (!in) {
        return;  // missing file → empty supplier list
    }

    std::string line;
    int lineNumber = 0;
    bool maybeHeader = true;
    while (std::getline(in, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (validation::trim(line).empty()) {
            continue;
        }
        const auto fields = splitCSVLine(line);
        if (maybeHeader && isHeaderRow(fields, "supplier_id")) {
            maybeHeader = false;
            continue;
        }
        maybeHeader = false;
        if (fields.size() < 3) {
            warnings_.push_back(lineLabel(name, lineNumber) + ": skipped (need supplier_id,name,contact)");
            continue;
        }
        const std::string id = validation::normalizeId(fields[0]);
        if (id.empty()) {
            warnings_.push_back(lineLabel(name, lineNumber) + ": skipped (empty supplier_id)");
            continue;
        }
        if (suppliers_.count(id) != 0) {
            warnings_.push_back(lineLabel(name, lineNumber) + ": skipped (duplicate supplier " + id + ")");
            continue;
        }
        suppliers_.emplace(id, std::make_shared<Supplier>(id, fields[1], fields[2]));
    }
}

void DataLayer::loadInventory() {
    const std::string name = "inventory.csv";
    const std::string path = filePath(name);
    std::ifstream in(path);
    if (!in) {
        return;  // missing file → empty inventory
    }

    std::string line;
    int lineNumber = 0;
    bool maybeHeader = true;
    while (std::getline(in, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (validation::trim(line).empty()) {
            continue;
        }
        const auto fields = splitCSVLine(line);
        if (maybeHeader && isHeaderRow(fields, "sku_vin")) {
            maybeHeader = false;
            continue;
        }
        maybeHeader = false;
        if (fields.size() < 4) {
            warnings_.push_back(lineLabel(name, lineNumber) +
                                ": skipped (need sku_vin,description,stock_quantity,supplier_id)");
            continue;
        }

        const std::string id = validation::normalizeId(fields[0]);
        std::string idError;
        if (!validation::isValidProductId(id, idError)) {
            warnings_.push_back(lineLabel(name, lineNumber) + ": skipped (" + idError + ")");
            continue;
        }
        if (inventory_.count(id) != 0) {
            warnings_.push_back(lineLabel(name, lineNumber) + ": skipped (duplicate SKU/VIN " + id + ")");
            continue;
        }

        int stock = 0;
        if (!validation::parseInt(fields[2], stock)) {
            warnings_.push_back(lineLabel(name, lineNumber) + ": skipped (stock is not an integer)");
            continue;
        }
        if (stock < 0) {
            warnings_.push_back(lineLabel(name, lineNumber) + ": skipped (negative stock)");
            continue;
        }

        std::shared_ptr<Supplier> supplier;
        const std::string supplierId = validation::normalizeId(fields[3]);
        if (!supplierId.empty()) {
            supplier = findSupplier(supplierId);
            if (!supplier) {
                warnings_.push_back(lineLabel(name, lineNumber) + ": unknown supplier '" + supplierId +
                                    "'; product stored without vendor");
            }
        }

        inventory_.emplace(id, std::make_shared<Product>(id, fields[1], stock, supplier));
    }
}

void DataLayer::loadTransactions() {
    const std::string name = "transactions.csv";
    const std::string path = filePath(name);
    std::ifstream in(path);
    if (!in) {
        return;  // missing file → empty history
    }

    std::string line;
    int lineNumber = 0;
    bool maybeHeader = true;
    while (std::getline(in, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (validation::trim(line).empty()) {
            continue;
        }
        const auto fields = splitCSVLine(line);
        if (maybeHeader && isHeaderRow(fields, "transaction_id")) {
            maybeHeader = false;
            continue;
        }
        maybeHeader = false;
        if (fields.size() < 5) {
            warnings_.push_back(lineLabel(name, lineNumber) +
                                ": skipped (need transaction_id,date,sku_vin,quantity_change,type)");
            continue;
        }

        const std::string tid = validation::normalizeId(fields[0]);
        if (tid.empty()) {
            warnings_.push_back(lineLabel(name, lineNumber) + ": skipped (empty transaction_id)");
            continue;
        }
        bool duplicate = false;
        for (const Transaction& existing : transactions_) {
            if (existing.getTransactionId() == tid) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            warnings_.push_back(lineLabel(name, lineNumber) + ": skipped (duplicate transaction " + tid + ")");
            continue;
        }

        int change = 0;
        if (!validation::parseInt(fields[3], change)) {
            warnings_.push_back(lineLabel(name, lineNumber) + ": skipped (quantity_change is not an integer)");
            continue;
        }

        const std::string sku = validation::normalizeId(fields[2]);
        auto product = findProduct(sku);
        if (!product) {
            warnings_.push_back(lineLabel(name, lineNumber) + ": skipped (unknown product " + sku + ")");
            continue;
        }

        // Do not replay quantity_change onto stock; inventory.csv is the current quantity.
        const std::string date = fields[1];
        const std::string type = validation::toUpper(fields[4]);
        transactions_.push_back(Transaction(tid, date, change, type, product));
    }
}

bool DataLayer::saveToCSV() {
    warnings_.clear();
    std::error_code ec;
    std::filesystem::create_directories(dataDir_, ec);
    if (ec) {
        warnings_.push_back("Cannot create data directory '" + dataDir_ + "': " + ec.message());
        return false;
    }
    bool ok = writeSuppliers();
    ok = writeInventory() && ok;
    ok = writeTransactions() && ok;
    return ok;
}

bool DataLayer::writeFile(const std::string& name, const std::string& contents) {
    std::ofstream out(filePath(name), std::ios::trunc);
    if (!out) {
        warnings_.push_back("Cannot write " + name);
        return false;
    }
    out << contents;
    if (!out) {
        warnings_.push_back("Failed while writing " + name);
        return false;
    }
    return true;
}

bool DataLayer::writeSuppliers() {
    auto list = allSuppliers();
    std::sort(list.begin(), list.end(), [](const std::shared_ptr<Supplier>& a,
                                           const std::shared_ptr<Supplier>& b) {
        return a->getSupplierId() < b->getSupplierId();
    });
    std::ostringstream out;
    out << "supplier_id,name,contact\n";
    for (const auto& supplier : list) {
        out << escapeCSV(supplier->getSupplierId()) << ',' << escapeCSV(supplier->getName()) << ','
            << escapeCSV(supplier->getContact()) << '\n';
    }
    return writeFile("suppliers.csv", out.str());
}

bool DataLayer::writeInventory() {
    auto list = allProducts();
    std::sort(list.begin(), list.end(), [](const std::shared_ptr<Product>& a,
                                           const std::shared_ptr<Product>& b) {
        return a->getSkuVin() < b->getSkuVin();
    });
    std::ostringstream out;
    out << "sku_vin,description,stock_quantity,supplier_id\n";
    for (const auto& product : list) {
        std::string supplierId;
        if (product->getSupplier()) {
            supplierId = product->getSupplier()->getSupplierId();
        }
        out << escapeCSV(product->getSkuVin()) << ',' << escapeCSV(product->getDescription()) << ','
            << product->getStockQuantity() << ',' << escapeCSV(supplierId) << '\n';
    }
    return writeFile("inventory.csv", out.str());
}

bool DataLayer::writeTransactions() {
    std::ostringstream out;
    out << "transaction_id,date,sku_vin,quantity_change,type\n";
    for (const Transaction& tx : transactions_) {
        std::string sku;
        if (tx.getProduct()) {
            sku = tx.getProduct()->getSkuVin();
        }
        out << escapeCSV(tx.getTransactionId()) << ',' << escapeCSV(tx.getDate()) << ',' << escapeCSV(sku)
            << ',' << tx.getQuantityChange() << ',' << escapeCSV(tx.getType()) << '\n';
    }
    return writeFile("transactions.csv", out.str());
}

bool DataLayer::addSupplier(std::shared_ptr<Supplier> supplier) {
    if (!supplier || supplier->getSupplierId().empty()) {
        return false;
    }
    const std::string& id = supplier->getSupplierId();
    if (suppliers_.count(id) != 0) {
        return false;
    }
    suppliers_.emplace(id, std::move(supplier));
    return true;
}

bool DataLayer::addProduct(std::shared_ptr<Product> product) {
    if (!product || product->getSkuVin().empty()) {
        return false;
    }
    const std::string& id = product->getSkuVin();
    if (inventory_.count(id) != 0) {
        return false;
    }
    inventory_.emplace(id, std::move(product));
    return true;
}

bool DataLayer::addTransaction(const Transaction& transaction) {
    if (transaction.getTransactionId().empty() || !transaction.getProduct()) {
        return false;
    }
    for (const Transaction& existing : transactions_) {
        if (existing.getTransactionId() == transaction.getTransactionId()) {
            return false;
        }
    }
    transactions_.push_back(transaction);
    return true;
}

bool DataLayer::removeProduct(const std::string& skuVin) {
    return inventory_.erase(skuVin) > 0;
}

std::shared_ptr<Product> DataLayer::findProduct(const std::string& skuVin) const {
    const auto it = inventory_.find(skuVin);
    if (it == inventory_.end()) {
        return nullptr;
    }
    return it->second;
}

std::shared_ptr<Supplier> DataLayer::findSupplier(const std::string& supplierId) const {
    const auto it = suppliers_.find(supplierId);
    if (it == suppliers_.end()) {
        return nullptr;
    }
    return it->second;
}

std::vector<std::shared_ptr<Product>> DataLayer::allProducts() const {
    std::vector<std::shared_ptr<Product>> out;
    out.reserve(inventory_.size());
    for (const auto& entry : inventory_) {
        out.push_back(entry.second);
    }
    return out;
}

std::vector<std::shared_ptr<Supplier>> DataLayer::allSuppliers() const {
    std::vector<std::shared_ptr<Supplier>> out;
    out.reserve(suppliers_.size());
    for (const auto& entry : suppliers_) {
        out.push_back(entry.second);
    }
    return out;
}
