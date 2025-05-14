cmake_minimum_required(VERSION 3.20)
project(DirectX12-Wrapper CXX)

# 変数にファイルを格納
file(GLOB_RECURSE WRAPPER_SOURCES "src/*.cpp")
file(GLOB_RECURSE WRAPPER_HEADERS "inc/*.hpp")

# hpp, cppファイルのまま扱うためのターゲット設定
add_library(${PROJECT_NAME} ${WRAPPER_SOURCES})

# 必要なパッケージが存在することを確認
find_package(directx12-agility CONFIG REQUIRED)
find_package(directx-dxc CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Microsoft::DirectX-Headers
    Microsoft::DirectX-Guids
    Microsoft::DirectX12-Agility
    Microsoft::DirectXShaderCompiler
    Microsoft::DirectXTex
    imgui::imgui
    d3d12
    dxgi
    dxcompiler
    dxguid
)

find_path(D3DX12_INCLUDE_DIRS "directx/d3dx12.h")
find_path(DIRECTX_INCLUDE_DIRS "directx")
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_path(IMGUI_INCLUDE_DIRS "imgui.h")

# ヘッダーファイルのあるディレクトリをインクルード
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/inc")
#↓めっちゃ重要！！！
target_include_directories(${PROJECT_NAME} PRIVATE "${DIRECTX_INCLUDE_DIRS}/directx")
target_include_directories(${PROJECT_NAME} PRIVATE ${IMGUI_INCLUDE_DIRS}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${TINYGLTF_INCLUDE_DIRS})

# 公開
# target_include_directories(${PROJECT_NAME}
#     PUBLIC
#         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#         $<INSTALL_INTERFACE:include>
# )

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

# NOTE : 順序に注意
# 外部ライブラリのインストールが終わって
# ディレクトリへのパスが変数に格納されてから
# add_subdirectoryする
# これによって下の階層に正しい値をもった変数がコピーされる
add_subdirectory(test/src)