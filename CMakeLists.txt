cmake_minimum_required(VERSION 4.0.0)

set(MNAME "VulkanJourney")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" VCPKG_CMAKE_PATH)
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_CMAKE_PATH}/scripts/buildsystems/vcpkg.cmake")

project(${MNAME} LANGUAGES CXX)


find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)


file(GLOB_RECURSE SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(${MNAME} ${SRC_FILES})

target_include_directories(${MNAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${MNAME}
    PRIVATE
        Vulkan::Vulkan
        glfw
        glm::glm
)

# 自动捕获 shader 编译
file(GLOB SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag
)

foreach(SHADER ${SHADER_FILES})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(SPIRV ${CMAKE_CURRENT_BINARY_DIR}/shaders/${FILE_NAME}.spv)

    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SHADER} -o ${SPIRV}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader ${FILE_NAME}"
    )

    list(APPEND SPIRV_OUTPUTS ${SPIRV})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPIRV_OUTPUTS})
add_dependencies(${MNAME} Shaders)