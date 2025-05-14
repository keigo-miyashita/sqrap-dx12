cmake_minimum_required(VERSION 3.20)
project(DirectX12-Wrapper CXX)

set(VCPKG_OVERLAY_PORTS "${CMAKE_SOURCE_DIR}/ports/directx-dxc;${CMAKE_SOURCE_DIR}/ports/directx12-agility" CACHE STRING "Overlay ports" FORCE)

# 必要なパッケージが存在することを確認
find_package(directx12-agility CONFIG REQUIRED)
find_package(directx-dxc CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_path(D3DX12_INCLUDE_DIRS "directx/d3dx12.h")
find_path(DIRECTX_INCLUDE_DIRS "directx")
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_path(IMGUI_INCLUDE_DIRS "imgui.h")
message(STATUS "Root CMakeList.txt TINYGLTF_INCLUDE_DIRS: ${TINYGLTF_INCLUDE_DIRS}")
message(STATUS "Root CMakeList.txt IMGUI_INCLUDE_DIRS: ${IMGUI_INCLUDE_DIRS}")

# hpp, cppファイルのまま扱うためのターゲット設定
# add_library(${PROJECT_NAME} )

# ヘッダーファイルのあるディレクトリをインクルード
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

# 変数にファイルを格納
file(GLOB_RECURSE WRAPPER_SOURCES "src/*.cpp")
file(GLOB_RECURSE WRAPPER_HEADERS "inc/*.hpp")

# hpp, cppファイルのまま扱うためのターゲット設定
# add_library(${PROJECT_NAME} ${WRAPPER_SOURCES})

# 公開
# target_include_directories(${PROJECT_NAME}
#     PUBLIC
#         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#         $<INSTALL_INTERFACE:include>
# )

message(STATUS "Root CMakeList.txt d3d12_SOURCE_DIR: ${d3d12_SOURCE_DIR}")

# includeディレクトリを設定
# INTERFACEはこのプロジェクト自体のビルドには
# このディレクトリを含まないことを指定
# 依存先のプロジェクトでのみ利用
# 今回のようにラッパーをつくるときに利用するのだろう
# NOTE : INTERFACEにより変数(ex. ${})の下階層への伝播に影響している可能性あり
# target_include_directories(${PROJECT_NAME}
#     PUBLIC
#         ${TINYGLTF_INCLUDE_DIRS}
#         "${dxc_SOURCE_DIR}/inc"
#         "${d3d12_SOURCE_DIR}/build/native/include"
# )

# リンクするライブラリを指定
# 同様にINTERFACEをつけている
# target_link_libraries(
#     ${PROJECT_NAME}
#     PUBLIC
#         d3d12
#         dxgi
#         Microsoft::DirectXTex
#         dxcompiler
#         imgui::imgui
# )

# NOTE : 順序に注意
# 外部ライブラリのインストールが終わって
# ディレクトリへのパスが変数に格納されてから
# add_subdirectoryする
# これによって下の階層に正しい値をもった変数がコピーされる
add_subdirectory(test/src)