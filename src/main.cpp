// CLIENT (interface) layer — src/main.cpp
//
// This CLI is a "client-server" design in-process, not over a network.
// There are no sockets and no HTTP. The numbered menu is the *client*:
// it collects keystrokes and prints results. InventoryService + DataLayer
// are the *server-side* logic and store. The operator never talks to CSV
// files directly.
//


#include "InventoryService.h"
#include "Validation.h"

#include <iostream>
#include <optional>
#include <string>

namespace {

enum class Role { User, Admin };

void printProduct(const Product& product) {
    std::cout << "ID: " << product.getSkuVin() << '\n'
              << "Description: " << product.getDescription() << '\n'
              << "Stock: " << product.getStockQuantity() << '\n';
    if (const auto supplier = product.getSupplier()) {
        std::cout << "Supplier: " << supplier->getSupplierId() << " (" << supplier->getName() << ")\n";
    } else {
        std::cout << "Supplier: (none)\n";
    }
}

void printProductRow(const Product& product) {
    std::string supplier = "(none)";
    if (product.getSupplier()) {
        supplier = product.getSupplier()->getSupplierId();
    }
    std::cout << product.getSkuVin() << " | stock " << product.getStockQuantity() << " | "
              << product.getDescription() << " | vendor " << supplier << '\n';
}

bool readLine(const std::string& prompt, std::string& out) {
    std::cout << prompt << std::flush;
    if (!std::getline(std::cin, out)) {
        return false;
    }
    out = validation::trim(out);
    return true;
}

bool readInt(const std::string& prompt, int& out) {
    std::string line;
    while (true) {
        if (!readLine(prompt, line)) {
            return false;
        }
        if (validation::parseInt(line, out)) {
            return true;
        }
        std::cout << "Please enter a whole number.\n";
    }
}

void showUserMenu() {
    std::cout << "\n=== Workshop user (retrieve only) ===\n"
              << "1. Search inventory by SKU/VIN\n"
              << "2. Sort products by stock (lowest first)\n"
              << "3. Low-stock report\n"
              << "4. List all products\n"
              << "5. List suppliers\n"
              << "6. Exit\n"
              << "Select an option: ";
}

void showAdminMenu() {
    std::cout << "\n=== Administrator (manage stock) ===\n"
              << "1. Add product (vehicle or part)\n"
              << "2. Edit product\n"
              << "3. Remove product\n"
              << "4. Record transaction (update stock)\n"
              << "5. Search inventory by SKU/VIN\n"
              << "6. Sort products by stock (lowest first)\n"
              << "7. Low-stock report\n"
              << "8. Add supplier\n"
              << "9. List suppliers\n"
              << "10. Link product to supplier\n"
              << "11. Save and exit\n"
              << "Select an option: ";
}

void handleAddProduct(InventoryService& service) {
    std::string id;
    std::string description;
    std::string supplierId;
    int stock = 0;
    if (!readLine("VIN (17 characters) or part SKU: ", id)) {
        return;
    }
    if (!readLine("Description: ", description)) {
        return;
    }
    if (!readInt("Initial stock quantity: ", stock)) {
        return;
    }
    if (!readLine("Supplier ID (blank if none): ", supplierId)) {
        return;
    }
    const OperationResult result = service.addProduct(id, description, stock, supplierId);
    std::cout << result.message << '\n';
}

void handleEditProduct(InventoryService& service) {
    std::string id;
    if (!readLine("SKU or VIN to edit: ", id)) {
        return;
    }
    const auto product = service.findProduct(id);
    if (!product) {
        std::cout << "Product not found.\n";
        return;
    }
    std::cout << "Current record:\n";
    printProduct(*product);

    std::string description;
    std::string stockText;
    std::string supplierId;
    if (!readLine("New description (blank to keep): ", description)) {
        return;
    }
    if (!readLine("New stock quantity (blank to keep): ", stockText)) {
        return;
    }
    if (!readLine("Supplier ID (blank to keep, - to unlink): ", supplierId)) {
        return;
    }

    std::optional<std::string> newDescription;
    if (!description.empty()) {
        newDescription = description;
    }
    std::optional<int> newStock;
    if (!stockText.empty()) {
        int stock = 0;
        if (!validation::parseInt(stockText, stock)) {
            std::cout << "Stock must be a whole number.\n";
            return;
        }
        newStock = stock;
    }
    const bool changeSupplier = !supplierId.empty();
    const OperationResult result =
        service.editProduct(id, newDescription, newStock, changeSupplier, supplierId);
    std::cout << result.message << '\n';
}

void handleRemoveProduct(InventoryService& service) {
    std::string id;
    if (!readLine("SKU or VIN to remove: ", id)) {
        return;
    }
    const auto product = service.findProduct(id);
    if (!product) {
        std::cout << "Product not found.\n";
        return;
    }
    std::cout << "About to remove:\n";
    printProduct(*product);
    std::string confirm;
    if (!readLine("Type YES to confirm: ", confirm)) {
        return;
    }
    if (validation::toUpper(confirm) != "YES") {
        std::cout << "Removal cancelled.\n";
        return;
    }
    std::cout << service.removeProduct(id).message << '\n';
}

void handleTransaction(InventoryService& service) {
    std::string sku;
    std::string tid;
    std::string date;
    std::string type;
    int change = 0;
    if (!readLine("Target SKU or VIN: ", sku)) {
        return;
    }
    if (!readLine("Transaction ID (e.g. TX001): ", tid)) {
        return;
    }
    if (!readLine("Date (YYYY-MM-DD): ", date)) {
        return;
    }
    if (!readInt("Quantity change (negative for sale/repair, positive for restock): ", change)) {
        return;
    }
    if (!readLine("Type (REPAIR, SALE, RESTOCK, ...): ", type)) {
        return;
    }
    std::cout << service.recordTransaction(tid, date, sku, change, type).message << '\n';
}

void handleSearch(InventoryService& service) {
    std::string id;
    if (!readLine("SKU or VIN to search: ", id)) {
        return;
    }
    const auto product = service.findProduct(id);
    if (!product) {
        std::cout << "Product not found.\n";
        return;
    }
    std::cout << "\nFound product:\n";
    printProduct(*product);
}

void handleSort(InventoryService& service) {
    if (service.productCount() == 0) {
        std::cout << "Inventory is empty.\n";
        return;
    }
    std::cout << "\n=== Products by stock (lowest first) ===\n";
    for (const auto& product : service.productsSortedByStock()) {
        printProductRow(*product);
    }
}

void handleListProducts(InventoryService& service) {
    if (service.productCount() == 0) {
        std::cout << "Inventory is empty.\n";
        return;
    }
    std::cout << "\n=== All products ===\n";
    for (const auto& product : service.productsSortedByStock()) {
        printProductRow(*product);
    }
}

void handleLowStock(InventoryService& service) {
    if (service.productCount() == 0) {
        std::cout << "Inventory is empty.\n";
        return;
    }
    int threshold = 0;
    if (!readInt("Low-stock threshold (products at or below this quantity): ", threshold)) {
        return;
    }
    const auto matches = service.lowStock(threshold);
    std::cout << "\n=== Low-stock report (threshold " << threshold << ") ===\n";
    if (matches.empty()) {
        std::cout << "No products at or below " << threshold << ".\n";
        return;
    }
    for (const auto& product : matches) {
        printProductRow(*product);
    }
}

void handleAddSupplier(InventoryService& service) {
    std::string id;
    std::string name;
    std::string contact;
    if (!readLine("Supplier ID: ", id)) {
        return;
    }
    if (!readLine("Name: ", name)) {
        return;
    }
    if (!readLine("Contact: ", contact)) {
        return;
    }
    std::cout << service.addSupplier(id, name, contact).message << '\n';
}

void handleListSuppliers(InventoryService& service) {
    const auto suppliers = service.allSuppliers();
    if (suppliers.empty()) {
        std::cout << "No suppliers on file.\n";
        return;
    }
    std::cout << "\n=== Suppliers ===\n";
    for (const auto& supplier : suppliers) {
        std::cout << supplier->getSupplierId() << " | " << supplier->getName() << " | "
                  << supplier->getContact() << '\n';
    }
}

void handleLinkSupplier(InventoryService& service) {
    std::string sku;
    std::string sid;
    if (!readLine("Product SKU or VIN: ", sku)) {
        return;
    }
    if (!readLine("Supplier ID: ", sid)) {
        return;
    }
    std::cout << service.linkProductToSupplier(sku, sid).message << '\n';
}

bool chooseRole(Role& role) {
    std::cout << "\nThis menu is the client. Logic and CSV storage run in-process as the server.\n"
              << "No network ports are opened.\n\n"
              << "Select interaction type:\n"
              << "1. Workshop user  (search, sort, low-stock, lists)\n"
              << "2. Administrator  (add/edit/remove, transactions, suppliers)\n";
    while (true) {
        std::string line;
        if (!readLine("Select 1 or 2: ", line)) {
            return false;
        }
        const std::string upper = validation::toUpper(line);
        if (line == "1" || upper == "USER") {
            role = Role::User;
            return true;
        }
        if (line == "2" || upper == "ADMIN" || upper == "ADMINISTRATOR") {
            role = Role::Admin;
            return true;
        }
        std::cout << "Enter 1 (user) or 2 (administrator).\n";
    }
}

bool runUserClient(InventoryService& service) {
    while (true) {
        showUserMenu();
        std::string choiceLine;
        if (!std::getline(std::cin, choiceLine)) {
            std::cout << "\nEnd of input. User session does not rewrite files.\n";
            return true;
        }
        choiceLine = validation::trim(choiceLine);
        int choice = 0;
        if (!validation::parseInt(choiceLine, choice)) {
            std::cout << "Invalid selection.\n";
            continue;
        }
        switch (choice) {
            case 1:
                handleSearch(service);
                break;
            case 2:
                handleSort(service);
                break;
            case 3:
                handleLowStock(service);
                break;
            case 4:
                handleListProducts(service);
                break;
            case 5:
                handleListSuppliers(service);
                break;
            case 6:
                return true;
            default:
                std::cout << "Invalid selection. Workshop users cannot change stock.\n";
                break;
        }
    }
}

bool runAdminClient(InventoryService& service) {
    while (true) {
        showAdminMenu();
        std::string choiceLine;
        if (!std::getline(std::cin, choiceLine)) {
            std::cout << "\nEnd of input. Saving...\n";
            std::cout << service.save().message << '\n';
            return true;
        }
        choiceLine = validation::trim(choiceLine);
        int choice = 0;
        if (!validation::parseInt(choiceLine, choice)) {
            std::cout << "Invalid selection.\n";
            continue;
        }
        switch (choice) {
            case 1:
                handleAddProduct(service);
                break;
            case 2:
                handleEditProduct(service);
                break;
            case 3:
                handleRemoveProduct(service);
                break;
            case 4:
                handleTransaction(service);
                break;
            case 5:
                handleSearch(service);
                break;
            case 6:
                handleSort(service);
                break;
            case 7:
                handleLowStock(service);
                break;
            case 8:
                handleAddSupplier(service);
                break;
            case 9:
                handleListSuppliers(service);
                break;
            case 10:
                handleLinkSupplier(service);
                break;
            case 11:
                std::cout << service.save().message << '\n';
                return true;
            default:
                std::cout << "Invalid selection.\n";
                break;
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::string dataDir = (argc > 1) ? argv[1] : ".";
    // Server-side logic object. The client (menus below) calls only this API.
    InventoryService service(dataDir);

    std::cout << "Automotive workshop / dealership inventory\n";
    std::cout << service.load().message << '\n';

    Role role = Role::User;
    if (!chooseRole(role)) {
        return 0;
    }
    if (role == Role::User) {
        std::cout << "Signed in as workshop user.\n";
        runUserClient(service);
    } else {
        std::cout << "Signed in as administrator.\n";
        runAdminClient(service);
    }
    return 0;
}
