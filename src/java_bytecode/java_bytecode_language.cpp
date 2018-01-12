/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include "java_bytecode_language.h"
#include "java_bytecode_synchronize.h"

#include <string>

#include <util/symbol_table.h>
#include <util/suffix.h>
#include <util/config.h>
#include <util/cmdline.h>
#include <util/expr_iterator.h>
#include <util/journalling_symbol_table.h>
#include <util/string2int.h>
#include <util/invariant.h>
#include <util/std_types.h>
#include <util/code_visitor.h>
#include <json/json_parser.h>

#include <goto-programs/class_hierarchy.h>

#include "java_bytecode_convert_class.h"
#include "java_bytecode_convert_method.h"
#include "java_bytecode_internal_additions.h"
#include "java_bytecode_instrument.h"
#include "java_bytecode_typecheck.h"
#include "java_entry_point.h"
#include "java_bytecode_parser.h"
#include "java_class_loader.h"
#include "java_utils.h"
#include <java_bytecode/ci_lazy_methods.h>
#include <java_bytecode/generate_java_generic_type.h>

#include "expr2java.h"

/// Consume options that are java bytecode specific.
/// \param Command:line options
/// \return None
void java_bytecode_languaget::get_language_options(const cmdlinet &cmd)
{
  assume_inputs_non_null=cmd.isset("java-assume-inputs-non-null");
  java_class_loader.use_core_models=!cmd.isset("no-core-models");
  string_refinement_enabled=cmd.isset("refine-strings");
  throw_runtime_exceptions=cmd.isset("java-throw-runtime-exceptions");
  if(cmd.isset("java-max-input-array-length"))
    object_factory_parameters.max_nondet_array_length=
      std::stoi(cmd.get_value("java-max-input-array-length"));
  if(cmd.isset("java-max-input-tree-depth"))
    object_factory_parameters.max_nondet_tree_depth=
      std::stoi(cmd.get_value("java-max-input-tree-depth"));
  if(cmd.isset("string-max-input-length"))
    object_factory_parameters.max_nondet_string_length=
      std::stoi(cmd.get_value("string-max-input-length"));
  object_factory_parameters.string_printable = cmd.isset("string-printable");
  if(cmd.isset("java-max-vla-length"))
    max_user_array_length=std::stoi(cmd.get_value("java-max-vla-length"));
  if(cmd.isset("lazy-methods-context-sensitive"))
    lazy_methods_mode=LAZY_METHODS_MODE_CONTEXT_SENSITIVE;
  else if(cmd.isset("lazy-methods"))
    lazy_methods_mode=LAZY_METHODS_MODE_CONTEXT_INSENSITIVE;
  else
    lazy_methods_mode=LAZY_METHODS_MODE_EAGER;

  if(cmd.isset("java-throw-runtime-exceptions"))
  {
    throw_runtime_exceptions = true;
    java_load_classes.insert(
        java_load_classes.end(),
        exception_needed_classes.begin(),
        exception_needed_classes.end());
  }
  if(cmd.isset("java-load-class"))
  {
    const auto &values = cmd.get_values("java-load-class");
    java_load_classes.insert(
        java_load_classes.end(), values.begin(), values.end());
  }

  const std::list<std::string> &extra_entry_points=
    cmd.get_values("lazy-methods-extra-entry-point");
  lazy_methods_extra_entry_points.insert(
    lazy_methods_extra_entry_points.end(),
    extra_entry_points.begin(),
    extra_entry_points.end());

  if(cmd.isset("java-cp-include-files"))
  {
    java_cp_include_files=cmd.get_value("java-cp-include-files");
    // load file list from JSON file
    if(java_cp_include_files[0]=='@')
    {
      jsont json_cp_config;
      if(parse_json(
           java_cp_include_files.substr(1),
           get_message_handler(),
           json_cp_config))
        throw "cannot read JSON input configuration for JAR loading";

      if(!json_cp_config.is_object())
        throw "the JSON file has a wrong format";
      jsont include_files=json_cp_config["jar"];
      if(!include_files.is_array())
         throw "the JSON file has a wrong format";

      // add jars from JSON config file to classpath
      for(const jsont &file_entry : include_files.array)
      {
        assert(file_entry.is_string() && has_suffix(file_entry.value, ".jar"));
        config.java.classpath.push_back(file_entry.value);
      }
    }
  }
  else
    java_cp_include_files=".*";

  language_options_initialized=true;
}

std::set<std::string> java_bytecode_languaget::extensions() const
{
  return { "class", "jar" };
}

void java_bytecode_languaget::modules_provided(std::set<std::string> &modules)
{
  // modules.insert(translation_unit(parse_path));
}

/// ANSI-C preprocessing
bool java_bytecode_languaget::preprocess(
  std::istream &instream,
  const std::string &path,
  std::ostream &outstream)
{
  // there is no preprocessing!
  return true;
}

bool java_bytecode_languaget::parse(
  std::istream &instream,
  const std::string &path)
{
  PRECONDITION(language_options_initialized);
  java_class_loader.set_message_handler(get_message_handler());
  java_class_loader.set_java_cp_include_files(java_cp_include_files);
  java_class_loader.add_load_classes(java_load_classes);

  // look at extension
  if(has_suffix(path, ".class"))
  {
    // override main_class
    main_class=java_class_loadert::file_to_class_name(path);
  }
  else if(has_suffix(path, ".jar"))
  {
    // build an object to potentially limit which classes are loaded
    java_class_loader_limitt class_loader_limit(
      get_message_handler(),
      java_cp_include_files);
    if(config.java.main_class.empty())
    {
      auto manifest=
        java_class_loader.jar_pool(class_loader_limit, path).get_manifest();
      std::string manifest_main_class=manifest["Main-Class"];

      // if the manifest declares a Main-Class line, we got a main class
      if(manifest_main_class!="")
        main_class=manifest_main_class;
    }
    else
      main_class=config.java.main_class;

    // do we have one now?
    if(main_class.empty())
    {
      status() << "JAR file without entry point: loading class files" << eom;
      java_class_loader.load_entire_jar(class_loader_limit, path);
      for(const auto &kv : java_class_loader.jar_map.at(path).entries)
        main_jar_classes.push_back(kv.first);
    }
    else
      java_class_loader.add_jar_file(path);
  }
  else
    UNREACHABLE;

  if(!main_class.empty())
  {
    status() << "Java main class: " << main_class << eom;
    java_class_loader(main_class);
  }

  return false;
}

bool java_bytecode_languaget::typecheck(
  symbol_tablet &symbol_table,
  const std::string &module)
{
  PRECONDITION(language_options_initialized);

  java_internal_additions(symbol_table);

  if(string_refinement_enabled)
    string_preprocess.initialize_conversion_table();

  // Must load java.lang.Object first to avoid stubbing
  // This ordering could alternatively be enforced by
  // moving the code below to the class loader.
  java_class_loadert::class_mapt::const_iterator it=
    java_class_loader.class_map.find("java.lang.Object");
  if(it!=java_class_loader.class_map.end())
  {
    if(java_bytecode_convert_class(
     it->second,
     symbol_table,
     get_message_handler(),
     max_user_array_length,
     method_bytecode,
     lazy_methods_mode,
     string_preprocess))
       return true;
  }

  // first generate a new struct symbol for each class and a new function symbol
  // for every method
  for(java_class_loadert::class_mapt::const_iterator
      c_it=java_class_loader.class_map.begin();
      c_it!=java_class_loader.class_map.end();
      c_it++)
  {
    if(c_it->second.parsed_class.name.empty())
      continue;

    if(
      java_bytecode_convert_class(
        c_it->second,
        symbol_table,
        get_message_handler(),
        max_user_array_length,
        method_bytecode,
        lazy_methods_mode,
        string_preprocess))
      return true;
  }

  // find and mark all implicitly generic class types
  // this can only be done once all the class symbols have been created
  for(const auto &c : java_class_loader.class_map)
  {
    if(c.second.parsed_class.name.empty())
      continue;
    try
    {
      mark_java_implicitly_generic_class_type(
        c.second.parsed_class.name, symbol_table);
    }
    catch(missing_outer_class_symbol_exceptiont &)
    {
      messaget::warning()
        << "Not marking class " << c.first
        << " implicitly generic due to missing outer class symbols"
        << messaget::eom;
    }
  }

  // Now incrementally elaborate methods
  // that are reachable from this entry point.
  if(lazy_methods_mode==LAZY_METHODS_MODE_CONTEXT_INSENSITIVE)
  {
    // ci: context-insensitive.
    if(do_ci_lazy_method_conversion(symbol_table, method_bytecode))
      return true;
  }
  else if(lazy_methods_mode==LAZY_METHODS_MODE_EAGER)
  {
    journalling_symbol_tablet journalling_symbol_table =
      journalling_symbol_tablet::wrap(symbol_table);
    // Convert all methods for which we have bytecode now
    for(const auto &method_sig : method_bytecode)
    {
      convert_single_method(method_sig.first, journalling_symbol_table);
    }
    // Now convert all newly added string methods
    for(const auto &fn_name : journalling_symbol_table.get_inserted())
    {
      if(string_preprocess.implements_function(fn_name))
        convert_single_method(fn_name, symbol_table);
    }
  }
  // Otherwise our caller is in charge of elaborating methods on demand.

  // now instrument runtime exceptions
  java_bytecode_instrument(
    symbol_table,
    throw_runtime_exceptions,
    get_message_handler());

  // now typecheck all
  bool res=java_bytecode_typecheck(
    symbol_table, get_message_handler(), string_refinement_enabled);
  // Deal with org.cprover.CProver.startThread() and endThread()
  code_visitort::callmapt map;
  code_visitort::callbackt f=
    std::bind(&java_bytecode_languaget::convert_threadblock,
      this, std::placeholders::_1, std::ref(symbol_table));
  map.insert({ID_function_call, f});
  code_visitort thread_block_visitor(map);
  thread_block_visitor.visit_symbols(symbol_table);

  return res;
}

bool java_bytecode_languaget::generate_support_functions(
  symbol_tablet &symbol_table)
{
  PRECONDITION(language_options_initialized);

  main_function_resultt res=
    get_main_symbol(symbol_table, main_class, get_message_handler());
  if(!res.is_success())
    return res.is_error();

  // Load the main function into the symbol table to get access to its
  // parameter names
  convert_lazy_method(res.main_function.name, symbol_table);

  // generate the test harness in __CPROVER__start and a call the entry point
  return
    java_entry_point(
      symbol_table,
      main_class,
      get_message_handler(),
      assume_inputs_non_null,
      object_factory_parameters,
      get_pointer_type_selector());
}

/// Uses a simple context-insensitive ('ci') analysis to determine which methods
/// may be reachable from the main entry point. In brief, static methods are
/// reachable if we find a callsite in another reachable site, while virtual
/// methods are reachable if we find a virtual callsite targeting a compatible
/// type *and* a constructor callsite indicating an object of that type may be
/// instantiated (or evidence that an object of that type exists before the main
/// function is entered, such as being passed as a parameter).
/// \par parameters: `symbol_table`: global symbol table
/// `method_bytecode`: map from method names to relevant symbol and
///   parsed-method objects.
/// \return Elaborates lazily-converted methods that may be reachable starting
///   from the main entry point (usually provided with the --function command-
///   line option) (side-effect on the symbol_table). Returns false on success.
bool java_bytecode_languaget::do_ci_lazy_method_conversion(
  symbol_tablet &symbol_table,
  method_bytecodet &method_bytecode)
{
  const method_convertert method_converter =
    [this, &symbol_table]
      (const irep_idt &function_id, ci_lazy_methods_neededt lazy_methods_needed)
    {
      return convert_single_method(
        function_id, symbol_table, std::move(lazy_methods_needed));
    };

  ci_lazy_methodst method_gather(
    symbol_table,
    main_class,
    main_jar_classes,
    lazy_methods_extra_entry_points,
    java_class_loader,
    java_load_classes,
    get_pointer_type_selector(),
    get_message_handler());

  return method_gather(symbol_table, method_bytecode, method_converter);
}

const select_pointer_typet &
  java_bytecode_languaget::get_pointer_type_selector() const
{
  PRECONDITION(pointer_type_selector.get()!=nullptr);
  return *pointer_type_selector;
}

/// Provide feedback to `language_filest` so that when asked for a lazy method,
/// it can delegate to this instance of java_bytecode_languaget.
/// \return Populates `methods` with the complete list of lazy methods that are
///   available to convert (those which are valid parameters for
///   `convert_lazy_method`)
void java_bytecode_languaget::methods_provided(id_sett &methods) const
{
  // Add all string solver methods to map
  string_preprocess.get_all_function_names(methods);
  // Add all concrete methods to map
  for(const auto &kv : method_bytecode)
    methods.insert(kv.first);
}

/// \brief Promote a lazy-converted method (one whose type is known but whose
/// body hasn't been converted) into a fully-elaborated one.
/// \remarks Amends the symbol table entry for function `function_id`, which
/// should be a method provided by this instance of `java_bytecode_languaget`
/// to have a value representing the method body identical to that produced
/// using eager method conversion.
/// \param function_id: method ID to convert
/// \param symtab: global symbol table
void java_bytecode_languaget::convert_lazy_method(
  const irep_idt &function_id,
  symbol_tablet &symtab)
{
  const symbolt &symbol = symtab.lookup_ref(function_id);
  if(symbol.value.is_not_nil())
    return;

  journalling_symbol_tablet symbol_table=
    journalling_symbol_tablet::wrap(symtab);

  convert_single_method(function_id, symbol_table);

  // Instrument runtime exceptions (unless symbol is a stub)
  if(symbol.value.is_not_nil())
  {
    java_bytecode_instrument_symbol(
      symbol_table,
      symbol_table.get_writeable_ref(function_id),
      throw_runtime_exceptions,
      get_message_handler());
  }

  // now typecheck this function
  java_bytecode_typecheck_updated_symbols(
    symbol_table, get_message_handler(), string_refinement_enabled);
}

/// \brief Convert a method (one whose type is known but whose body hasn't
///   been converted) but don't run typecheck, etc
/// \remarks Amends the symbol table entry for function `function_id`, which
///   should be a method provided by this instance of `java_bytecode_languaget`
///   to have a value representing the method body.
/// \param function_id: method ID to convert
/// \param symbol_table: global symbol table
/// \param needed_lazy_methods: optionally a collection of needed methods to
///   update with any methods touched during the conversion
bool java_bytecode_languaget::convert_single_method(
  const irep_idt &function_id,
  symbol_table_baset &symbol_table,
  optionalt<ci_lazy_methods_neededt> needed_lazy_methods)
{
  const symbolt &symbol = symbol_table.lookup_ref(function_id);

  // Nothing to do if body is already loaded
  if(symbol.value.is_not_nil())
    return false;

  // Get bytecode for specified function if we have it
  method_bytecodet::opt_reft cmb = method_bytecode.get(function_id);

  // Check if have a string solver implementation
  if(string_preprocess.implements_function(function_id))
  {
    symbolt &symbol = symbol_table.get_writeable_ref(function_id);
    // Load parameter names from any extant bytecode before filling in body
    if(cmb)
    {
      java_bytecode_initialize_parameter_names(
        symbol, cmb->get().method.local_variable_table, symbol_table);
    }
    // Populate body of the function with code generated by string preprocess
    exprt generated_code =
      string_preprocess.code_for_function(symbol, symbol_table);
    INVARIANT(
      generated_code.is_not_nil(), "Couldn't retrieve code for string method");
    // String solver can make calls to functions that haven't yet been seen.
    // Add these to the needed_lazy_methods collection
    if(needed_lazy_methods)
    {
      for(const_depth_iteratort it = generated_code.depth_cbegin();
          it != generated_code.depth_cend();
          ++it)
      {
        if(it->id() == ID_code)
        {
          const auto fn_call = expr_try_dynamic_cast<code_function_callt>(*it);
          if(!fn_call)
            continue;
          // Only support non-virtual function calls for now, if string solver
          // starts to introduce virtual function calls then we will need to
          // duplicate the behavior of java_bytecode_convert_method where it
          // handles the invokevirtual instruction
          const symbol_exprt &fn_sym =
            expr_dynamic_cast<symbol_exprt>(fn_call->function());
          needed_lazy_methods->add_needed_method(fn_sym.get_identifier());
        }
      }
    }
    symbol.value = generated_code;
    return false;
  }

  // No string solver implementation, check if have bytecode for it
  if(cmb)
  {
    java_bytecode_convert_method(
      symbol_table.lookup_ref(cmb->get().class_id),
      cmb->get().method,
      symbol_table,
      get_message_handler(),
      max_user_array_length,
      std::move(needed_lazy_methods),
      string_preprocess);
    return false;
  }

  return true;
}


/// This function will recursively look for a thread block.
/// In this context, a thread block is defined as any code
/// that is in between a call to CProver.startThread:(I)V
/// and CProver.endThread:(I)V. Once, a thread block is found
/// the aforementioned calls are instrumented.
///
/// TODO: check for mismatches in startThread and endThread.
///
/// \param code: codet, should be ID_function_call
/// \param symbol_table: a symbol table
void java_bytecode_languaget::convert_threadblock(codet &code,
  symbol_tablet &symbol_table)
{
  PRECONDITION(code.get_statement()==ID_function_call);
  namespacet ns(symbol_table);
  const std::string& next_thread_id="__CPROVER_next_thread_id";
  const std::string& thread_id="__CPROVER_thread_id";
  std::string fname="";
  code_function_callt &f_code=to_code_function_call(code);
  from_expr(f_code.function(), fname, ns);

  // get function name and see if it
  // matches org.cprover.start() or org.cprover.end()
  if(fname == "org.cprover.CProver.startThread:(I)V")
  {
    INVARIANT(f_code.arguments().size()==1,
      "ERROR: CProver.startThread invalid number of arguments");

    // build id's, used to construct appropriate labels.
    const exprt &expr=f_code.arguments()[0];
    const std::string& v_str=expr.op0().get_string(ID_value);
    mp_integer v=binary2integer(v_str, false);
    // java does not have labels so choosing these names is safe.
    const std::string &lbl1="TS_1_"+integer2string(v);
    const std::string &lbl2="TS_2_"+integer2string(v);

    // instrument the following code:
    //
    // A: codet(ID_start_thread) --> TS_1_<ID>
    // B: goto : TS_2_<ID>
    // C: label (TS_1_<ID>)
    // C.1 codet(ID_atomic_begin)
    // D: __CPROVER_next_thread_id+=1;
    // E: __CPROVER_thread_id=__CPROVER_next_thread_id
    // F.1 codet(ID_atomic_end)

    const symbolt& next_symbol=
      add_or_get_symbol(symbol_table, next_thread_id, next_thread_id,
        java_int_type(), from_integer(mp_zero, java_int_type()), false, true);
    const symbolt& current_symbol=
      add_or_get_symbol(symbol_table, thread_id, thread_id,
        java_int_type(), from_integer(mp_zero, java_int_type()), true, true);

    codet tmp_a(ID_start_thread);
    tmp_a.set(ID_destination, lbl1);
    code_gotot tmp_b(lbl2);
    code_labelt tmp_c(lbl1);
    tmp_c.op0()=codet(ID_skip);

    exprt plus(ID_plus, java_int_type());
    plus.copy_to_operands(next_symbol.symbol_expr());
    plus.copy_to_operands(from_integer(1, java_int_type()));
    code_assignt tmp_d(next_symbol.symbol_expr(), plus);
    code_assignt tmp_e(current_symbol.symbol_expr(), next_symbol.symbol_expr());

    code_blockt block;
    block.add(tmp_a);
    block.add(tmp_b);
    block.add(tmp_c);
    block.add(codet(ID_atomic_begin));
    block.add(tmp_d);
    block.add(tmp_e);
    block.add(codet(ID_atomic_end));

    block.add_source_location()=code.source_location();
    code=block;
  }
  else if(fname == "org.cprover.CProver.endThread:(I)V")
  {
    INVARIANT(f_code.arguments().size()==1,
      "ERROR: CProver.endThread invalid number of arguments");

    // build id, used to construct appropriate labels.
    const exprt &expr=f_code.arguments()[0];
    const std::string& v_str=expr.op0().get_string(ID_value);
    mp_integer v=binary2integer(v_str, false);
    // java does not have labels so this this is safe.
    const std::string &lbl2="TS_2_"+integer2string(v);

    // instrument the following code:
    // F: codet(ID_end_thread)
    // G: label (TS_2_<ID>)
    codet tmp_f(ID_end_thread);
    code_labelt tmp_g(lbl2);
    tmp_g.op0()=codet(ID_skip);

    code_blockt block;
    block.add(tmp_f);
    block.add(tmp_g);

    block.add_source_location()=code.source_location();
    code=block;
  }
  else if(fname == "org.cprover.CProver.getCurrentThreadID:()I")
  {
    INVARIANT(f_code.arguments().size()==0,
      "ERROR: CProver.getCurrentThreadID invalid number of arguments");

    const symbolt& current_symbol=
      add_or_get_symbol(symbol_table, thread_id, thread_id,
        java_int_type(), from_integer(mp_zero, java_int_type()), true, true);

    code_assignt code_assign(f_code.lhs(), current_symbol.symbol_expr());
    code_assign.add_source_location()=code.source_location();
    code=code_assign;
  }
  return;
}


bool java_bytecode_languaget::final(symbol_table_baset &symbol_table)
{
  PRECONDITION(language_options_initialized);

  java_bytecode_synchronizet synchronizer(symbol_table);

  return recreate_initialize(
    symbol_table,
    main_class,
    get_message_handler(),
    assume_inputs_non_null,
    object_factory_parameters,
    get_pointer_type_selector());
}

void java_bytecode_languaget::show_parse(std::ostream &out)
{
  java_class_loader(main_class).output(out);
}

std::unique_ptr<languaget> new_java_bytecode_language()
{
  return util_make_unique<java_bytecode_languaget>();
}

bool java_bytecode_languaget::from_expr(
  const exprt &expr,
  std::string &code,
  const namespacet &ns)
{
  code=expr2java(expr, ns);
  return false;
}

bool java_bytecode_languaget::from_type(
  const typet &type,
  std::string &code,
  const namespacet &ns)
{
  code=type2java(type, ns);
  return false;
}

bool java_bytecode_languaget::to_expr(
  const std::string &code,
  const std::string &module,
  exprt &expr,
  const namespacet &ns)
{
  #if 0
  expr.make_nil();

  // no preprocessing yet...

  std::istringstream i_preprocessed(code);

  // parsing

  java_bytecode_parser.clear();
  java_bytecode_parser.filename="";
  java_bytecode_parser.in=&i_preprocessed;
  java_bytecode_parser.set_message_handler(message_handler);
  java_bytecode_parser.grammar=java_bytecode_parsert::EXPRESSION;
  java_bytecode_parser.mode=java_bytecode_parsert::GCC;
  java_bytecode_scanner_init();

  bool result=java_bytecode_parser.parse();

  if(java_bytecode_parser.parse_tree.items.empty())
    result=true;
  else
  {
    expr=java_bytecode_parser.parse_tree.items.front().value();

    result=java_bytecode_convert(expr, "", message_handler);

    // typecheck it
    if(!result)
      result=java_bytecode_typecheck(expr, message_handler, ns);
  }

  // save some memory
  java_bytecode_parser.clear();

  return result;
  #endif

  return true; // fail for now
}

java_bytecode_languaget::~java_bytecode_languaget()
{
}

/// Utility method
/// Adds a new symbol to the symbol table if
/// it doesn't exist. Otherwise, returns
/// existing one.
symbolt java_bytecode_languaget::add_or_get_symbol(
  symbol_tablet& symbol_table,
  const irep_idt& name,
  const irep_idt& base_name,
  const typet& type,
  const exprt& value,
  const bool is_thread_local,
  const bool is_static_lifetime)
{
  const symbolt* psymbol = nullptr;
  namespacet ns(symbol_table);
  ns.lookup(name, psymbol);
  if(psymbol != nullptr)
    return *psymbol;
  symbolt new_symbol;
  new_symbol.name=name;
  new_symbol.pretty_name=name;
  new_symbol.base_name=base_name;
  new_symbol.type=type;
  new_symbol.value=value;
  new_symbol.is_lvalue=true;
  new_symbol.is_state_var=true;
  new_symbol.is_static_lifetime=is_static_lifetime;
  new_symbol.is_thread_local=is_thread_local;
  new_symbol.mode=ID_java;
  symbol_table.add(new_symbol);
  return new_symbol;
}

