# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources})

#idf_component_register(SRCS "mesh_light.c"
#                            "mesh_main.c"
#                    PRIV_REQUIRES esp_wifi esp_driver_gpio esp_driver_ledc nvs_flash
#                    INCLUDE_DIRS "." "include")
