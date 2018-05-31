message(STATUS "Downloading java-models-library...")
include("${CBMC_SOURCE_DIR}/../cmake/DownloadProject.cmake")
download_project(PROJ java_models_library
    URL https://github.com/diffblue/java-models-library/archive/master.zip
    PATCH_COMMAND cmake -E copy_directory "${JBMC_BINARY_DIR}/../../java_models_library-src/src"
      "${JBMC_SOURCE_DIR}/java_bytecode/library/src"
)

find_package(Java REQUIRED)
include(UseJava)

set(CMAKE_JAVA_COMPILE_FLAGS -sourcepath "src" -d "classes" -XDignore.symbol.file)

# create a target for the executable performing the .jar -> .inc conversion
add_executable(java-converter converter.cpp)

# create a target 'core-models.jar' that depends on all .java files in src/
file(GLOB_RECURSE java_sources "src/*.java")
add_jar("core-models" ${java_sources})

# define a cmake variable with the full path of the .inc file
set(JAVA_CORE_MODELS_INC "${CMAKE_CURRENT_BINARY_DIR}/java_core_models.inc")
