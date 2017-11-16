find_package(Java REQUIRED)
include(UseJava)
set(CMAKE_JAVA_COMPILE_FLAGS -sourcepath "library/src" -d "library/classes" -XDignore.symbol.file)
file(GLOB_RECURSE java_sources "*.java")
set(JAR_NAME "core-models")
add_jar(${JAR_NAME} ${java_sources})
get_filename_component(CORE_MODELS_JAR "library/core-models.jar" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(JAVA_CORE_MODELS_INC "library/java_core_models.inc" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE jars "*.jar")
add_executable(java-converter library/converter.cpp)

add_custom_target(core_models_files)
add_dependencies(core_models_files ${JAR_NAME})
add_custom_command(TARGET core_models_files
                   PRE_BUILD
                   COMMAND java-converter JAVA_CORE_MODELS core-models.jar > ${JAVA_CORE_MODELS_INC}
                   )

file(GLOB sources "*.cpp")
file(GLOB_RECURSE headers "*.h")
set_source_files_properties("library/java_core_models.inc" GENERATED)

add_library(java_bytecode ${c_sources} ${sources} ${headers} )
add_dependencies(java_bytecode core_models_files)
target_sources(java_bytecode PUBLIC ${sources} ${headers})

generic_includes(java_bytecode)

target_link_libraries(java_bytecode util goto-programs miniz json)
