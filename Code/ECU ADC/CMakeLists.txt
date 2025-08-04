cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME ECU_ADC)

include("cmake/gcc-arm-none-eabi.cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

enable_language(C ASM)

add_executable(${CMAKE_PROJECT_NAME})

# === Add STM32CubeMX sources ===
add_subdirectory(cmake/stm32cubemx)

# === Add user sources ===

# Add top-level application code (like app.c)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    src/app.c
)

# Automatically collect all module sources (*.c files in src/modules/**)
file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS
    src/modules/*.c
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${MODULE_SOURCES})

# === Include paths ===

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    include             # global project-wide headers
    src                 # for app.h
    src/modules         # base module dir
)

# Optional: include subdirectories of modules explicitly
file(GLOB MODULE_DIRS LIST_DIRECTORIES true src/modules/*)
foreach(dir ${MODULE_DIRS})
    if(IS_DIRECTORY ${dir})
        target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${dir})
    endif()
endforeach()

# === Preprocessor defines (optional) ===
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Example: -DDEBUG
)

# === Link with CubeMX output ===
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
)

# === User-defined library paths (optional) ===
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user library paths if needed
)