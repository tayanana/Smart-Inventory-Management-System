#pragma once

#include "Product.h"
#include "Supplier.h"
#include "Transaction.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Data layer: in-memory store plus CSV load/save.
//
// Ownership:
//   - suppliers_ and inventory_ hold shared_ptr values (RAII, no new/delete).
//   - transactions_ hold Transaction objects, each with a shared_ptr<Product>
//     so history outlives map removal.
// Lookup is average O(1) by ID via unordered_map; history is chronological vector.
class DataLayer {
public:
    explicit DataLayer(std::string dataDirectory = ".");

    void loadFromCSV();
    bool saveToCSV();

    bool addSupplier(std::shared_ptr<Supplier> supplier);
    bool addProduct(std::shared_ptr<Product> product);
    bool addTransaction(const Transaction& transaction);
    bool removeProduct(const std::string& skuVin);

    std::shared_ptr<Product> findProduct(const std::string& skuVin) const;
    std::shared_ptr<Supplier> findSupplier(const std::string& supplierId) const;

    std::vector<std::shared_ptr<Product>> allProducts() const;
    std::vector<std::shared_ptr<Supplier>> allSuppliers() const;
    const std::vector<Transaction>& allTransactions() const { return transactions_; }

    std::size_t productCount() const { return inventory_.size(); }
    std::size_t supplierCount() const { return suppliers_.size(); }
    std::size_t transactionCount() const { return transactions_.size(); }

    const std::vector<std::string>& warnings() const { return warnings_; }
    const std::string& dataDirectory() const { return dataDir_; }

private:
    std::string dataDir_;
    std::unordered_map<std::string, std::shared_ptr<Supplier>> suppliers_;
    std::unordered_map<std::string, std::shared_ptr<Product>> inventory_;
    std::vector<Transaction> transactions_;
    std::vector<std::string> warnings_;

    std::string filePath(const std::string& name) const;
    std::vector<std::string> splitCSVLine(const std::string& line) const;
    static std::string escapeCSV(const std::string& field);
    static bool isHeaderRow(const std::vector<std::string>& fields, const std::string& firstColumn);

    void loadSuppliers();
    void loadInventory();
    void loadTransactions();
    bool writeSuppliers();
    bool writeInventory();
    bool writeTransactions();
    bool writeFile(const std::string& name, const std::string& contents);
};
