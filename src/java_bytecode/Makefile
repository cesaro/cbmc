SRC = bytecode_info.cpp \
      character_refine_preprocess.cpp \
      ci_lazy_methods.cpp \
      ci_lazy_methods_needed.cpp \
      expr2java.cpp \
      jar_file.cpp \
      java_bytecode_convert_class.cpp \
      java_bytecode_convert_method.cpp \
      java_bytecode_instrument.cpp \
      java_bytecode_internal_additions.cpp \
      java_bytecode_language.cpp \
      java_bytecode_parse_tree.cpp \
      java_bytecode_parser.cpp \
      java_bytecode_synchronize.cpp \
      java_bytecode_typecheck.cpp \
      java_bytecode_typecheck_code.cpp \
      java_bytecode_typecheck_expr.cpp \
      java_bytecode_typecheck_type.cpp \
      java_class_loader.cpp \
      java_class_loader_limit.cpp \
      java_entry_point.cpp \
      java_local_variable_table.cpp \
      java_object_factory.cpp \
      java_pointer_casts.cpp \
      java_root_class.cpp \
      java_string_library_preprocess.cpp \
      java_types.cpp \
      java_utils.cpp \
      generate_java_generic_type.cpp \
      mz_zip_archive.cpp \
      select_pointer_type.cpp \
      # Empty last line

INCLUDES= -I ..

include ../config.inc
include ../common


SOURCE_DIR := library/src
BINARY_DIR := library/classes

FIND := find

JAVAC := javac
JFLAGS := -sourcepath $(SOURCE_DIR) -d $(BINARY_DIR) -XDignore.symbol.file

CLASSPATH := SOURCE_DIR

all_javas := $(shell $(FIND) $(SOURCE_DIR) -name '*.java')
all_classes := $(patsubst $(SOURCE_DIR)/%.java,$(BINARY_DIR)/%.class,$(all_javas))

$(BINARY_DIR):
	mkdir -p $(BINARY_DIR)

.SUFFIXES: .java .class

$(BINARY_DIR)/%.class : $(SOURCE_DIR)/%.java $(BINARY_DIR)
	$(JAVAC) $(JFLAGS) $(patsubst $(BINARY_DIR)/%.class,$(SOURCE_DIR)/%.java,$@)

JAR := jar
JARFLAGS := -cf

core-models.jar: $(all_classes)
	$(JAR) $(JARFLAGS) $@ -C $(BINARY_DIR) .

CLEANFILES = java_bytecode$(LIBEXT) core-models.jar converter

clean : clean_

clean_ :
	$(RM) -Rf $(BINARY_DIR)

all: java_bytecode$(LIBEXT)

converter: library/converter.cpp
	$(CXX) $(CP_CXXFLAGS) -o $@ $<

java_class_loader.o: java_core_models.inc java_class_loader.cpp
	$(CXX) -c $(CP_CXXFLAGS) -o $@ java_class_loader.cpp

java_core_models.inc: converter core-models.jar
	./converter JAVA_CORE_MODELS < core-models.jar > $@
###############################################################################

java_bytecode$(LIBEXT): $(OBJ)
	$(LINKLIB)
