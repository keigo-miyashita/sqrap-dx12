# Sqrap for DirectX 12

**Sqrap** is a personal wrapper of low level graphics API, and this repository is for DirectX 12, designed to simplify development while supporting modern GPU features — including **Work Graphs**.

## Features

- Work-in-progress
- **Support for Work Graphs** (requires Windows 11, a recent GPU and driver)
- Vulkan support is planned for future version

## Requirements
- **Visual Studio 2022**
- **CMake** (version 3.21 or higher)
  
### Work Graph Specific Requirements

This wrapper support Work Graph, so you need:

- **NVIDIA GPU Driver** version **566.36 or later**
- **Windows Developer Mode** turned **enabled**  
  Go to: `Settings` → `Systems` → `For developers` → Enable **Developer Mode**

## Usage

1. Make directory and add sqrap as submodule
```powershell
mkdir PROJECT_DIR
git init
git submodule add https://github.com/keigo-miyashita/sqrap-dx12.git
```
2. Add main.cpp and copy the required file from template directory
```
PROJECT_DIR/
├── src/
│ ├── main.cpp
├── .gitignore
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg-configuration.json
├── vcpkg.json
```
