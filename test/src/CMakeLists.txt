# 必要なパッケージが存在していることを確認
find_package(directx-headers CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# ビルドの設定
add_executable(${PROJECT_NAME}-test-src)
target_compile_features(${PROJECT_NAME}-test-src PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME}-test-src PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /utf-8>)
target_sources(${PROJECT_NAME}-test-src PRIVATE main.cpp "TestApplication.cpp" ${WRAPPER_SOURCES} "SampleScene.cpp")

# ライブラリのリンク
target_link_libraries(${PROJECT_NAME}-test-src PRIVATE Microsoft::DirectX-Headers Microsoft::DirectX-Guids Microsoft::DirectX12-Agility)
target_link_libraries(${PROJECT_NAME}-test-src PRIVATE Microsoft::DirectXShaderCompiler)
target_link_libraries(${PROJECT_NAME}-test-src PRIVATE Microsoft::DirectXTex)
target_link_libraries(${PROJECT_NAME}-test-src PRIVATE imgui::imgui)
target_link_libraries(${PROJECT_NAME}-test-src PRIVATE d3d12)
target_link_libraries(${PROJECT_NAME}-test-src PRIVATE dxgi)
target_link_libraries(${PROJECT_NAME}-test-src PRIVATE dxcompiler)
target_link_libraries(${PROJECT_NAME}-test-src PRIVATE dxguid)

# インクルードディレクトリへのパスを追加
target_include_directories(${PROJECT_NAME}-test-src PRIVATE "${CMAKE_SOURCE_DIR}/inc")
#target_include_directories(${PROJECT_NAME}-test-src PRIVATE "${D3DX12_INCLUDE_DIRS}")
#↓めっちゃ重要！！！
target_include_directories(${PROJECT_NAME}-test-src PRIVATE "${DIRECTX_INCLUDE_DIRS}/directx")
target_include_directories(${PROJECT_NAME}-test-src PRIVATE ${IMGUI_INCLUDE_DIRS}/include)
target_include_directories(${PROJECT_NAME}-test-src PRIVATE ${TINYGLTF_INCLUDE_DIRS})

get_cmake_property(_variableNames VARIABLES)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

# d3d12の機能を使うためのファイルを実行ファイル以下のディレクトリにコピー
if(TARGET Microsoft::DirectX12-Agility)
#        file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/D3D12")
        add_custom_command(TARGET ${PROJECT_NAME}-test-src POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}-test-src>/D3D12")
        add_custom_command(TARGET ${PROJECT_NAME}-test-src POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Core,IMPORTED_LOCATION_RELEASE> $<TARGET_FILE_DIR:${PROJECT_NAME}-test-src>/D3D12
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG> $<TARGET_FILE_DIR:${PROJECT_NAME}-test-src>/D3D12
           COMMAND_EXPAND_LISTS
        )
    endif()

    # message(STATUS "${VCPKG_INSTALLED}/${VCPKG_TARGET_TRIPLET}/bin/dxcompiler.dll")
    message(STATUS "dxcompiler.dll path: ${VCPKG_INSTALLED}/${VCPKG_TARGET_TRIPLET}/bin/dxcompiler.dll")
    message(STATUS "vcpkg_installed path: " ${VCPKG_INSTALLED})
 # dxilはなぜか設定しないと参照されないので、実行ファイルと同じ階層にコピー
add_custom_command(TARGET ${PROJECT_NAME}-test-src POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/dxcompiler.dll"
  "${CMAKE_SOURCE_DIR}/external/dll/dxil.dll"
  "$<TARGET_FILE_DIR:${PROJECT_NAME}-test-src>")

# シェーダー、モデル用のディレクトリを設定
set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shader")
add_compile_definitions(SHADER_DIR=L"${SHADER_DIR}")
set(MODEL_DIR "${CMAKE_SOURCE_DIR}/external/model")
add_compile_definitions(MODEL_DIR="${MODEL_DIR}")