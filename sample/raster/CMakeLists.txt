add_executable(${PROJECT_NAME}-sample-raster)
target_compile_features(${PROJECT_NAME}-sample-raster PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME}-sample-raster PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /utf-8>)
target_sources(${PROJECT_NAME}-sample-raster PRIVATE main.cpp "SampleApplication.cpp" "SampleScene.cpp")

target_link_libraries(
    ${PROJECT_NAME}-sample-raster PRIVATE
    sqrap-dx12
)

target_include_directories(${PROJECT_NAME}-sample-raster PRIVATE "${PROJECT_SOURCE_DIR}/inc")

if(TARGET Microsoft::DirectX12-Agility)
    add_custom_command(TARGET ${PROJECT_NAME}-sample-raster POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}-sample-raster>/${D3D12_DLL_DIR}")
    add_custom_command(TARGET ${PROJECT_NAME}-sample-raster POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Core,IMPORTED_LOCATION_RELEASE> $<TARGET_FILE_DIR:${PROJECT_NAME}-sample-raster>/${D3D12_DLL_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG> $<TARGET_FILE_DIR:${PROJECT_NAME}-sample-raster>/${D3D12_DLL_DIR}
        COMMAND_EXPAND_LISTS
    )
endif()

add_custom_command(TARGET ${PROJECT_NAME}-sample-raster POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/dxcompiler.dll"
  "${PROJECT_SOURCE_DIR}/external/dll/dxil.dll"
  "$<TARGET_FILE_DIR:${PROJECT_NAME}-sample-raster>")

if(MSVC)
    # リリースビルドの場合は WinMain を使う
    set_target_properties(${PROJECT_NAME}-sample-raster PROPERTIES
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
    )
endif()

set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/")
add_compile_definitions(SHADER_DIR=L"${SHADER_DIR}")
set(MODEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../model/")
add_compile_definitions(MODEL_DIR="${MODEL_DIR}")
