cmake_minimum_required(VERSION 3.20)
# project(${PROJECT_NAME} CXX)

# add_executable(${PROJECT_NAME} "src/main.cpp")
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /utf-8>)

add_subdirectory(sqrap-dx12)

target_link_libraries(
	${PROJECT_NAME} PUBLIC
	sqrap-dx12
)

target_include_directories(
	${PROJECT_NAME} PUBLIC
	sqrap-dx12/inc
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${D3D12_DLL_DIR}")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
	"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/D3D12Core.dll"
	"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/D3D12Core.pdb"
	"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin/d3d12SDKLayers.dll"
	"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin/d3d12SDKLayers.dll"
	$<TARGET_FILE_DIR:${PROJECT_NAME}>/${D3D12_DLL_DIR}
    COMMAND_EXPAND_LISTS
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/dxcompiler.dll"
  "${PROJECT_SOURCE_DIR}/external/dll/dxil.dll"
  "$<TARGET_FILE_DIR:${PROJECT_NAME}>")