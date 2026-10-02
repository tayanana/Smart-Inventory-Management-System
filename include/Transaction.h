#pragma once

#include "Product.h"

#include <memory>
#include <string>
#include <utility>

// One stock movement (repair consumption, sale, restock, ...).
// Holds shared_ptr<Product> so the history stays valid if the product is
// later removed from the active inventory map (no dangling raw pointer).
class Transaction {
private:
    std::string transaction_id;
    std::string date;
    int quantity_change;
    std::string type;
    std::shared_ptr<Product> product;

public:
    Transaction(std::string t_id, std::string dt, int change, std::string t_type,
                std::shared_ptr<Product> prod)
        : transaction_id(std::move(t_id)),
          date(std::move(dt)),
          quantity_change(change),
          type(std::move(t_type)),
          product(std::move(prod)) {}

    const std::string& getTransactionId() const { return transaction_id; }
    const std::string& getDate() const { return date; }
    int getQuantityChange() const { return quantity_change; }
    const std::string& getType() const { return type; }
    std::shared_ptr<Product> getProduct() const { return product; }
};
