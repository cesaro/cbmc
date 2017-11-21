file(GLOB sources "*.cpp")
file(GLOB headers "*.h")
get_filename_component(JAVA_CORE_LIBRARY_PATH "library" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(CORE_MODELS_JAR "library/core-models.jar" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE jars "*.jar")
add_executable(java-converter library/converter.cpp)

add_custom_target(core_models_classes)
add_custom_command(TARGET core_models_classes
                   PRE_BUILD
                   COMMAND make -C "${JAVA_CORE_LIBRARY_PATH}"
                   )

add_library(java_bytecode ${sources} ${headers} )
add_dependencies(java_bytecode core_models_classes)
target_sources(java_bytecode ${sources} ${headers})

generic_includes(java_bytecode)

target_link_libraries(java_bytecode util goto-programs miniz json)
