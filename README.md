# Smart-Inventory-Management-System

# Automotive Workshop and Dealership Inventory System

This repository contains the source code for the Smart Inventory Management System, developed for the General Programming with C/C++ course (DLBMINPAPCC01_E).

## Project Scope
The application manages inventory for vehicles and spare parts, records transactions to update stock quantities, and generates reports. Data is persisted to local CSV files.

## Build Instructions (macOS)
The project uses CMake as the build system and Clang for compilation.

1. Create a build directory:
   mkdir build
   cd build

2. Generate the build files:
   cmake ..

3. Compile the executable:
   make

4. Run the program:
   ./AutoInventorySystem
