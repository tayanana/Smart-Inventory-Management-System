#pragma once

#include "Supplier.h"

#include <memory>
#include <string>
#include <utility>

// One inventory item: a vehicle (17-character VIN) or a spare part (internal SKU).
// The optional supplier pointer is shared with DataLayer's supplier map.
class Product {
private:
    std::string sku_vin;
    std::string description;
    int stock_quantity;
    std::shared_ptr<Supplier> supplier;

public:
    Product(std::string id, std::string desc, int stock, std::shared_ptr<Supplier> supp)
        : sku_vin(std::move(id)),
          description(std::move(desc)),
          stock_quantity(stock),
          supplier(std::move(supp)) {}

    const std::string& getSkuVin() const { return sku_vin; }
    const std::string& getDescription() const { return description; }
    int getStockQuantity() const { return stock_quantity; }
    std::shared_ptr<Supplier> getSupplier() const { return supplier; }

    void setDescription(std::string desc) { description = std::move(desc); }
    void setSupplier(std::shared_ptr<Supplier> supp) { supplier = std::move(supp); }

    // Returns false and leaves stock unchanged when qty would be negative.
    bool setStockQuantity(int qty) {
        if (qty < 0) {
            return false;
        }
        stock_quantity = qty;
        return true;
    }

    // Logic layer must refuse a change that would take stock below zero
    // before calling this. No clamp is applied here.
    void adjustStock(int amount) { stock_quantity += amount; }
};
