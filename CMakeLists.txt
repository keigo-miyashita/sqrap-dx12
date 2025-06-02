cmake_minimum_required(VERSION 3.20)

project(sqrap-dx12 CXX)

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

# target_include_directoriesをPUBLICにすることでライブラリを使う側に書く必要がなくなる
# ヘッダーファイルのあるディレクトリをインクルード
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/inc")
#↓めっちゃ重要！！！directx-headersの.hをincludeするために設定
target_include_directories(${PROJECT_NAME} PUBLIC "${DIRECTX_INCLUDE_DIRS}/directx")
target_include_directories(${PROJECT_NAME} PUBLIC ${IMGUI_INCLUDE_DIRS}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${TINYGLTF_INCLUDE_DIRS})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_compile_options(${PROJECT_NAME} PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /utf-8>)

if (NOT DEFINED ENABLE_VERSION_ASSIGNMENT)
    set(D3D12_DLL_DIR "D3D12")
    set(D3D12_SDK_VERSION 715)
    set(D3D12_SDK_PATH "\".\\\\${D3D12_DLL_DIR}\\\\\"")
endif()

# プリプロセッサ定義として渡す
target_compile_definitions(${PROJECT_NAME} PUBLIC
    D3D12_SDK_VERSION_MACRO=${D3D12_SDK_VERSION}
    D3D12_SDK_PATH_MACRO=${D3D12_SDK_PATH}
)

# NOTE : 順序に注意
# 外部ライブラリのインストールが終わって
# ディレクトリへのパスが変数に格納されてから
# add_subdirectoryする
# これによって下の階層に正しい値をもった変数がコピーされる
add_subdirectory(sample/raster)
add_subdirectory(sample/raytracing)