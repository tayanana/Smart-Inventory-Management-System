#include "InventoryService.h"
#include "Validation.h"

#include <algorithm>
#include <sstream>
#include <utility>

InventoryService::InventoryService(std::string dataDirectory) : store_(std::move(dataDirectory)) {}

OperationResult InventoryService::load() {
    store_.loadFromCSV();
    std::ostringstream message;
    message << "Loaded " << store_.productCount() << " product(s), " << store_.supplierCount()
            << " supplier(s), " << store_.transactionCount() << " transaction(s) from '"
            << store_.dataDirectory() << "'.";
    if (!store_.warnings().empty()) {
        message << "\nLoad warnings:";
        for (const std::string& warning : store_.warnings()) {
            message << "\n  - " << warning;
        }
    }
    return {true, message.str()};
}

OperationResult InventoryService::save() {
    if (!store_.saveToCSV()) {
        std::string message = "Save failed.";
        for (const std::string& warning : store_.warnings()) {
            message += "\n  - " + warning;
        }
        return {false, message};
    }
    return {true, "Saved inventory, suppliers, and transactions to '" + store_.dataDirectory() + "'."};
}

OperationResult InventoryService::addSupplier(const std::string& id, const std::string& name,
                                              const std::string& contact) {
    const std::string normalized = validation::normalizeId(id);
    const std::string trimmedName = validation::trim(name);
    if (normalized.empty()) {
        return {false, "Supplier ID cannot be empty."};
    }
    if (trimmedName.empty()) {
        return {false, "Supplier name cannot be empty."};
    }
    if (store_.findSupplier(normalized)) {
        return {false, "Duplicate supplier ID: " + normalized};
    }
    if (!store_.addSupplier(
            std::make_shared<Supplier>(normalized, trimmedName, validation::trim(contact)))) {
        return {false, "Could not add supplier " + normalized + "."};
    }
    return {true, "Supplier " + normalized + " added."};
}

OperationResult InventoryService::addProduct(const std::string& id, const std::string& description,
                                             int stock, const std::string& supplierId) {
    const std::string normalized = validation::normalizeId(id);
    std::string error;
    if (!validation::isValidProductId(normalized, error)) {
        return {false, error};
    }
    const std::string desc = validation::trim(description);
    if (desc.empty()) {
        return {false, "Description cannot be empty."};
    }
    if (stock < 0) {
        return {false, "Initial stock cannot be negative."};
    }
    if (store_.findProduct(normalized)) {
        return {false, "Duplicate SKU/VIN: " + normalized};
    }

    std::shared_ptr<Supplier> supplier;
    const std::string supplierKey = validation::normalizeId(supplierId);
    if (!supplierKey.empty()) {
        supplier = store_.findSupplier(supplierKey);
        if (!supplier) {
            return {false, "Unknown supplier ID '" + supplierKey + "'. Add the supplier first, or leave it blank."};
        }
    }

    if (!store_.addProduct(std::make_shared<Product>(normalized, desc, stock, supplier))) {
        return {false, "Could not add product " + normalized + "."};
    }
    return {true, "Product " + normalized + " added."};
}

OperationResult InventoryService::editProduct(const std::string& id,
                                              const std::optional<std::string>& description,
                                              const std::optional<int>& stock, bool changeSupplier,
                                              const std::string& supplierId) {
    const std::string normalized = validation::normalizeId(id);
    auto product = store_.findProduct(normalized);
    if (!product) {
        return {false, "Product not found: " + normalized};
    }

    if (description) {
        const std::string desc = validation::trim(*description);
        if (desc.empty()) {
            return {false, "Description cannot be empty."};
        }
        product->setDescription(desc);
    }
    if (stock) {
        if (!product->setStockQuantity(*stock)) {
            return {false, "Stock cannot be negative."};
        }
    }
    if (changeSupplier) {
        const std::string supplierKey = validation::normalizeId(supplierId);
        if (supplierKey.empty() || supplierKey == "-") {
            product->setSupplier(nullptr);
        } else {
            auto supplier = store_.findSupplier(supplierKey);
            if (!supplier) {
                return {false, "Unknown supplier ID '" + supplierKey + "'."};
            }
            product->setSupplier(supplier);
        }
    }
    return {true, "Product " + normalized + " updated."};
}

OperationResult InventoryService::removeProduct(const std::string& id) {
    const std::string normalized = validation::normalizeId(id);
    if (!store_.findProduct(normalized)) {
        return {false, "Product not found: " + normalized};
    }
    store_.removeProduct(normalized);
    return {true, "Removed " + normalized +
                      " from active inventory. Open transactions still hold the product in memory."};
}

OperationResult InventoryService::linkProductToSupplier(const std::string& productId,
                                                        const std::string& supplierId) {
    const std::string sku = validation::normalizeId(productId);
    const std::string sid = validation::normalizeId(supplierId);
    auto product = store_.findProduct(sku);
    if (!product) {
        return {false, "Product not found: " + sku};
    }
    if (sid.empty()) {
        return {false, "Supplier ID cannot be empty."};
    }
    auto supplier = store_.findSupplier(sid);
    if (!supplier) {
        return {false, "Unknown supplier ID '" + sid + "'."};
    }
    product->setSupplier(supplier);
    return {true, "Linked " + sku + " to supplier " + sid + " (" + supplier->getName() + ")."};
}

OperationResult InventoryService::recordTransaction(const std::string& transactionId,
                                                    const std::string& date, const std::string& skuVin,
                                                    int quantityChange, const std::string& type) {
    const std::string tid = validation::normalizeId(transactionId);
    const std::string sku = validation::normalizeId(skuVin);
    const std::string trimmedDate = validation::trim(date);
    const std::string trimmedType = validation::toUpper(validation::trim(type));

    if (tid.empty()) {
        return {false, "Transaction ID cannot be empty."};
    }
    if (trimmedDate.empty()) {
        return {false, "Date cannot be empty."};
    }
    if (trimmedType.empty()) {
        return {false, "Transaction type cannot be empty."};
    }

    auto product = store_.findProduct(sku);
    if (!product) {
        return {false, "Product not found: " + sku};
    }

    if (product->getStockQuantity() + quantityChange < 0) {
        return {false, "Transaction would take stock below 0 (current stock: " +
                           std::to_string(product->getStockQuantity()) + ")."};
    }

    Transaction tx(tid, trimmedDate, quantityChange, trimmedType, product);
    if (!store_.addTransaction(tx)) {
        return {false, "Duplicate transaction ID: " + tid};
    }
    product->adjustStock(quantityChange);
    return {true, "Transaction recorded. New stock for " + sku + ": " +
                      std::to_string(product->getStockQuantity()) + "."};
}

std::shared_ptr<Product> InventoryService::findProduct(const std::string& id) const {
    return store_.findProduct(validation::normalizeId(id));
}

std::shared_ptr<Supplier> InventoryService::findSupplier(const std::string& id) const {
    return store_.findSupplier(validation::normalizeId(id));
}

std::vector<std::shared_ptr<Product>> InventoryService::productsSortedByStock() const {
    auto products = store_.allProducts();
    std::sort(products.begin(), products.end(),
              [](const std::shared_ptr<Product>& a, const std::shared_ptr<Product>& b) {
                  if (a->getStockQuantity() != b->getStockQuantity()) {
                      return a->getStockQuantity() < b->getStockQuantity();
                  }
                  return a->getSkuVin() < b->getSkuVin();
              });
    return products;
}

std::vector<std::shared_ptr<Product>> InventoryService::lowStock(int threshold) const {
    std::vector<std::shared_ptr<Product>> matches;
    for (const auto& product : productsSortedByStock()) {
        if (product->getStockQuantity() <= threshold) {
            matches.push_back(product);
        }
    }
    return matches;
}

std::vector<std::shared_ptr<Supplier>> InventoryService::allSuppliers() const {
    auto suppliers = store_.allSuppliers();
    std::sort(suppliers.begin(), suppliers.end(),
              [](const std::shared_ptr<Supplier>& a, const std::shared_ptr<Supplier>& b) {
                  return a->getSupplierId() < b->getSupplierId();
              });
    return suppliers;
}

std::size_t InventoryService::productCount() const { return store_.productCount(); }
std::size_t InventoryService::supplierCount() const { return store_.supplierCount(); }
std::size_t InventoryService::transactionCount() const { return store_.transactionCount(); }
const std::string& InventoryService::dataDirectory() const { return store_.dataDirectory(); }
const std::vector<std::string>& InventoryService::loadWarnings() const { return store_.warnings(); }
