#pragma once

#include <string>

// Shared ID / VIN / integer checks used by the logic layer and CSV loader.
namespace validation {

std::string trim(const std::string& text);
std::string normalizeId(std::string id);
std::string toUpper(std::string text);

// ISO 3779: exactly 17 alphanumeric characters, excluding I, O, and Q.
bool isValidVIN(const std::string& vin);

// Empty IDs fail. IDs of length 17 must be valid VINs. Shorter part SKUs
// may use letters, digits, underscore, and hyphen.
bool isValidProductId(const std::string& id, std::string& error);

bool parseInt(const std::string& text, int& out);

}  // namespace validation
