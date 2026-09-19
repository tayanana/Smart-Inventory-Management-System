#include "Validation.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace validation {

std::string trim(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(start, end - start);
}

std::string toUpper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return text;
}

std::string normalizeId(std::string id) {
    return toUpper(trim(id));
}

bool isValidVIN(const std::string& vin) {
    if (vin.size() != 17) {
        return false;
    }
    for (unsigned char c : vin) {
        if (!std::isalnum(c)) {
            return false;
        }
        const char upper = static_cast<char>(std::toupper(c));
        if (upper == 'I' || upper == 'O' || upper == 'Q') {
            return false;
        }
    }
    return true;
}

bool isValidProductId(const std::string& id, std::string& error) {
    if (id.empty()) {
        error = "Product ID cannot be empty.";
        return false;
    }
    if (id.size() == 17 && !isValidVIN(id)) {
        error = "Invalid VIN: 17-character IDs must follow ISO 3779 (A-H, J-N, P, R-Z, 0-9; no I, O, or Q).";
        return false;
    }
    for (unsigned char c : id) {
        if (std::isalnum(c) || c == '_' || c == '-') {
            continue;
        }
        error = "Product ID may contain letters, digits, underscore, and hyphen only.";
        return false;
    }
    return true;
}

bool parseInt(const std::string& text, int& out) {
    const std::string trimmed = trim(text);
    if (trimmed.empty()) {
        return false;
    }
    try {
        std::size_t idx = 0;
        const int value = std::stoi(trimmed, &idx);
        if (idx != trimmed.size()) {
            return false;
        }
        out = value;
        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace validation
