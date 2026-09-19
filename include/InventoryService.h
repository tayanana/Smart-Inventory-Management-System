#pragma once

#include "DataLayer.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

// Logic layer: validation, stock rules, search, sort, and reports.
// The CLI talks only to this type, not to the CSV maps directly.
struct OperationResult {
    bool ok = false;
    std::string message;
};

class InventoryService {
public:
    explicit InventoryService(std::string dataDirectory = ".");

    OperationResult load();
    OperationResult save();

    OperationResult addSupplier(const std::string& id, const std::string& name,
                                const std::string& contact);
    OperationResult addProduct(const std::string& id, const std::string& description, int stock,
                               const std::string& supplierId);
    OperationResult editProduct(const std::string& id,
                                const std::optional<std::string>& description,
                                const std::optional<int>& stock, bool changeSupplier,
                                const std::string& supplierId);
    OperationResult removeProduct(const std::string& id);
    OperationResult linkProductToSupplier(const std::string& productId,
                                          const std::string& supplierId);
    OperationResult recordTransaction(const std::string& transactionId, const std::string& date,
                                      const std::string& skuVin, int quantityChange,
                                      const std::string& type);

    std::shared_ptr<Product> findProduct(const std::string& id) const;
    std::shared_ptr<Supplier> findSupplier(const std::string& id) const;

    std::vector<std::shared_ptr<Product>> productsSortedByStock() const;
    std::vector<std::shared_ptr<Product>> lowStock(int threshold) const;
    std::vector<std::shared_ptr<Supplier>> allSuppliers() const;

    std::size_t productCount() const;
    std::size_t supplierCount() const;
    std::size_t transactionCount() const;
    const std::string& dataDirectory() const;
    const std::vector<std::string>& loadWarnings() const;

private:
    DataLayer store_;
};
