# add_executable(thermocouple_lookup thermocouple_lookup.c)

# target_include_directories(thermocouple_lookup PRIVATE
#     ${CMAKE_SOURCE_DIR}/src/modules/thermocouple_k
# )

# # Add the thermocouple_k source file(s)
# target_sources(thermocouple_lookup PRIVATE
#     ${CMAKE_SOURCE_DIR}/src/modules/thermocouple_k/thermocouple_k.c
# )

# # Link standard C library (usually automatic, but explicit for clarity)
# target_link_libraries(thermocouple_lookup PRIVATE m)

# cmake_minimum_required(VERSION 3.10)
# project(ThermocoupleLookup)

# add_executable(thermocouple_lookup thermocouple_lookup.c)

# target_include_directories(thermocouple_lookup PRIVATE
#     ../../src/modules/thermocouple_k
# )

# # Add the thermocouple_k source file(s) - correct path
# target_sources(thermocouple_lookup PRIVATE
#     ../../src/modules/thermocouple_k/thermocouple_k.c
# )

# # Link standard C library (usually automatic, but explicit for clarity)
# target_link_libraries(thermocouple_lookup PRIVATE m)

cmake_minimum_required(VERSION 3.10)
project(ThermocoupleLookup)

add_executable(thermocouple_lookup thermocouple_lookup.c)

target_include_directories(thermocouple_lookup PRIVATE
    ../../src/modules/thermocouple_k
)

# Add the thermocouple_k source file(s) - correct path
target_sources(thermocouple_lookup PRIVATE
    ../../src/modules/thermocouple_k/thermocouple_k.c
)