#pragma once

#include <string>
#include <utility>

// Vendor that supplies vehicles or spare parts.
// DataLayer owns Supplier instances through std::shared_ptr (RAII).
class Supplier {
private:
    std::string supplier_id;
    std::string name;
    std::string contact;

public:
    Supplier(std::string id, std::string n, std::string c)
        : supplier_id(std::move(id)), name(std::move(n)), contact(std::move(c)) {}

    const std::string& getSupplierId() const { return supplier_id; }
    const std::string& getName() const { return name; }
    const std::string& getContact() const { return contact; }

    void setName(std::string n) { name = std::move(n); }
    void setContact(std::string c) { contact = std::move(c); }
};
