/*******************************************************************\

Module: Java Convert Thread blocks

Author: Kurt Degiogrio, kurt.degiorgio@diffblue.com

\*******************************************************************/

#include "java_bytecode_convert_threadblock.h"
#include "expr2java.h"
#include "java_types.h"

#include <util/expr_iterator.h>
#include <util/namespace.h>
#include <util/cprover_prefix.h>
#include <util/std_types.h>
#include <util/arith_tools.h>

// Disable linter to allow an std::string constant.
const std::string next_thread_id = CPROVER_PREFIX "next_thread_id";// NOLINT(*)
const std::string thread_id = CPROVER_PREFIX "thread_id";// NOLINT(*)

/// Adds a new symbol to the symbol table if it doesn't exist. Otherwise,
/// returns the existing one.
/// /param name: name of the symbol to be generated
/// /param base_name: base name of the symbol to be generated
/// /param type: type of the symbol to be generated
/// /param value: initial value of the symbol to be generated
/// /param is_thread_local: if true this symbol will be set as thread local
/// /param is_static_lifetime: if true this symbol will be set as static
/// /return returns new or existing symbol.
static symbolt add_or_get_symbol(
  symbol_tablet &symbol_table,
  const irep_idt &name,
  const irep_idt &base_name,
  const typet &type,
  const exprt &value,
  const bool is_thread_local,
  const bool is_static_lifetime)
{
  const symbolt* psymbol = nullptr;
  namespacet ns(symbol_table);
  ns.lookup(name, psymbol);
  if(psymbol != nullptr)
    return *psymbol;
  symbolt new_symbol;
  new_symbol.name = name;
  new_symbol.pretty_name = name;
  new_symbol.base_name = base_name;
  new_symbol.type = type;
  new_symbol.value = value;
  new_symbol.is_lvalue = true;
  new_symbol.is_state_var = true;
  new_symbol.is_static_lifetime = is_static_lifetime;
  new_symbol.is_thread_local = is_thread_local;
  new_symbol.mode = ID_java;
  symbol_table.add(new_symbol);
  return new_symbol;
}

/// Retrieve the first label identifier. This is used to mark the start of
/// a thread block.
/// /param id: unique thread block identifier
/// /return: fully qualified label identifier
static const std::string get_first_label_id(const std::string &id)
{
  return CPROVER_PREFIX "_THREAD_ENTRY_" + id;
}

/// Retrieve the second label identifier. This is used to mark the end of
/// a thread block.
/// /param id: unique thread block identifier
/// /return: fully qualified label identifier
static const std::string get_second_label_id(const std::string &id)
{
  return CPROVER_PREFIX "_THREAD_EXIT_" + id;
}

/// Retrieves a thread block identifier from a function call to
/// CProver.startThread:(I)V or CProver.endThread:(I)V
/// /param code: function call to CProver.startThread or CProver.endThread
/// /return: unique thread block identifier
static const std::string get_thread_block_identifier(
  const code_function_callt &f_code)
{
  PRECONDITION(f_code.arguments().size() == 1);
  const exprt &expr = f_code.arguments()[0];
  mp_integer lbl_id = binary2integer(expr.op0().get_string(ID_value), false);
  return integer2string(lbl_id);
}

/// Create a temporary variable
/// /param symbol_table: a symbol table
/// /param prefix: prefix of the name of the temporary variable
/// /param name: base name of the temporary variable, to be append with "_tmp"
/// /param type: type of the temporary variable
/// /return returns new variable
static symbol_exprt tmp_variable(
  symbol_tablet &symbol_table,
  const std::string &prefix,
  const std::string &name,
  const typet &type)
{
  irep_idt base_name = name + "_tmp";
  irep_idt identifier = prefix + "::" + id2string(base_name);

  auxiliary_symbolt tmp_symbol;
  tmp_symbol.base_name = base_name;
  tmp_symbol.is_static_lifetime = false;
  tmp_symbol.mode = ID_java;
  tmp_symbol.name = identifier;
  tmp_symbol.type = type;
  symbol_table.add(tmp_symbol);

  symbol_exprt result(identifier, type);
  result.set(ID_C_base_name, base_name);
  return result;
}

/// Creates a monitorenter/monitorexit code_function_callt for
/// the given synchronization object.
/// \param symbol_table: a symbol table
/// \param is_enter: Indicates whether we are creating a monitorenter or exit.
/// \param object: An expression representing object  the object that need to
/// perform a monitorenter/monitorexit operation
static codet get_monitor_call(
  symbol_tablet &symbol_table,
  bool is_enter,
  const exprt &object)
{
  code_typet type;
  type.return_type() = void_typet();
  type.parameters().resize(1);
  type.parameters()[0].type() = java_reference_type(void_typet());
  std::string symbol;

  if(is_enter)
    symbol="java::java.lang.Object.monitorenter:(Ljava/lang/Object;)V";
  else
    symbol="java::java.lang.Object.monitorexit:(Ljava/lang/Object;)V";

  symbol_tablet::symbolst::const_iterator it
    = symbol_table.symbols.find(symbol);

  // If the functions for monitorenter or monitorexit do not exist
  // (this can only happen if the java-models libary was not downloaded).
  // We implement them as skips because symex would crash, since it cannot find
  // the function in the symbol table
  if(it == symbol_table.symbols.end())
    return code_skipt();

  // Otherwise we create a function call
  code_function_callt call;
  call.function() = symbol_exprt(symbol, type);
  call.lhs().make_nil();
  call.arguments().push_back(object);
  return call;
}

/// Introduces a monitorexit before every return recursively
/// \param code current element to check
/// \param monitorexit codet to insert before the return.
static void monitor_exits(
  codet &code,
  const codet &monitorexit)
{
  const irep_idt &statement=code.get_statement();
  if(statement == ID_return)
  {
    // Relace the return with a monitor exit plus return block
    code_blockt return_block;
    return_block.add(monitorexit);
    return_block.move_to_operands(code);
    code = return_block;
  }
  else if((statement == ID_label)
    || (statement == ID_block)
    || (statement == ID_decl_block))
  {
    // If label or block found, explore the code inside the block
    Forall_operands(it, code)
    {
      codet &sub_code = to_code(*it);
      monitor_exits(sub_code, monitorexit);
    }
  }
}

/// Transforms the codet stored in in the value field of \p symbol. This
/// expected to be a codet that needs to be synchronized, the transformation
/// makes it thread-safe as described in the documentation of function
/// convert_synchronized_methods
///
/// The resulting thread-safe codet is stored in the value field of \p symbol.
///
/// \param symbol_table: a symbol_table
/// \param symbol: writeable symbol hosting code to synchronized
/// \param sync_object: object to use as a lock
static void instrument_synchronized_code(
  symbol_tablet &symbol_table,
  symbolt &symbol,
  const exprt &sync_object)
{
    codet &code = to_code(symbol.value);

    // Get interposition code for the monitor lock/unlock
    codet monitorenter = get_monitor_call(symbol_table, true, sync_object);
    codet monitorexit = get_monitor_call(symbol_table, false, sync_object);

    // Create a unique catch label and empty throw type (i.e any)
    // and catch-push them at the beginning of the code (i.e begin try)
    code_push_catcht catch_push;
    irep_idt handler("pc_synchronized_catch");
    irep_idt exception_id;
    code_push_catcht::exception_listt &exception_list=
        catch_push.exception_list();
    exception_list.push_back(
      code_push_catcht::exception_list_entryt(exception_id, handler));

    // Create a catch-pop to indicate the end of the try block
    code_pop_catcht catch_pop;

    // Create the finally block with the same label targeted in the catch-push
    symbol_exprt catch_var =
      tmp_variable(
        symbol_table,
        id2string(symbol.name),
        "caught_synchronized_exception",
        java_reference_type(symbol_typet("java::java.lang.Throwable")));
    code_landingpadt catch_statement(catch_var);
    codet catch_instruction = catch_statement;
    code_labelt catch_label(handler, code_blockt());
    code_blockt &catch_block = to_code_block(catch_label.code());
    catch_block.add(catch_instruction);
    catch_block.add(monitorexit);

    // Re-throw exception
    side_effect_expr_throwt throw_expr;
    throw_expr.copy_to_operands(catch_var);
    catch_block.add(code_expressiont(throw_expr));

    // Write a monitorexit before every return
    monitor_exits(code, monitorexit);

    // Wrap the code into a try finally
    code_blockt try_block;
    try_block.move_to_operands(monitorenter);
    try_block.move_to_operands(catch_push);
    try_block.move_to_operands(code);
    try_block.move_to_operands(catch_pop);
    try_block.move_to_operands(catch_label);
    code = try_block;
}

/// Transforms the codet stored in in \p f_code, which is a call to function
/// CProver.startThread:(I)V into a block of code as described by the
/// documentation of function convert_thread_block
///
/// The resulting code_blockt is stored in the output parameter \p code.
///
/// \param f_code: function call to CProver.startThread:(I)V
/// \param [out] code: resulting transformation
/// \param symbol_table: a symbol table
static void instrument_start_thread(
  const code_function_callt &f_code,
  codet &code,
  symbol_tablet &symbol_table)
{
  PRECONDITION(f_code.arguments().size() == 1);

  // Create global variable __CPROVER_next_thread_id. Used as a counter
  // in-order to to assign ids to new threads.
  const symbolt &next_symbol =
    add_or_get_symbol(
        symbol_table, next_thread_id, next_thread_id,
        java_int_type(),
        from_integer(0, java_int_type()), false, true);

  // Create thread-local variable __CPROVER_thread_id. Holds the id of
  // the thread.
  const symbolt &current_symbol =
    add_or_get_symbol(
        symbol_table, thread_id, thread_id, java_int_type(),
        from_integer(0, java_int_type()), true, true);

  // Construct appropriate labels.
  // Note: java does not have labels so this should be safe.
  const std::string &thread_block_id = get_thread_block_identifier(f_code);
  const std::string &lbl1 = get_first_label_id(thread_block_id);
  const std::string &lbl2 = get_second_label_id(thread_block_id);

  // Instrument the following codet's:
  //
  // A: codet(id=ID_start_thread, destination=__CPROVER_THREAD_ENTRY_<ID>)
  // B: codet(id=ID_goto, destination=__CPROVER_THREAD_EXIT_<ID>)
  // C: codet(id=ID_label, label=__CPROVER_THREAD_ENTRY_<ID>)
  // C.1: codet(id=ID_atomic_begin)
  // D: CPROVER_next_thread_id += 1;
  // E: __CPROVER_thread_id = __CPROVER_next_thread_id;
  // F: codet(id=ID_atomic_end)

  codet tmp_a(ID_start_thread);
  tmp_a.set(ID_destination, lbl1);
  code_gotot tmp_b(lbl2);
  code_labelt tmp_c(lbl1);
  tmp_c.op0() = codet(ID_skip);

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
  block.add_source_location() = f_code.source_location();

  code = block;
}

/// Transforms the codet stored in in \p f_code, which is a call to function
/// CProver.endThread:(I)V into a block of code as described by the
/// documentation of the function convert_thread_block.
///
/// The resulting code_blockt is stored in the output parameter \p code.
///
/// \param f_code: function call to CProver.endThread:(I)V
/// \param [out] code: resulting transformation
/// \param symbol_table: a symbol table
static void instrument_endThread(
  const code_function_callt &f_code,
  codet &code,
  symbol_tablet symbol_table)
{
  PRECONDITION(f_code.arguments().size() == 1);

  // Build id, used to construct appropriate labels.
  // Note: java does not have labels so this should be safe
  const std::string &thread_block_id = get_thread_block_identifier(f_code);
  const std::string &lbl2 = get_second_label_id(thread_block_id);

  // Instrument the following code:
  //
  // A: codet(id=ID_end_thread)
  // B: codet(id=ID_label,label= __CPROVER_THREAD_EXIT_<ID>)
  codet tmp_a(ID_end_thread);
  code_labelt tmp_b(lbl2);
  tmp_b.op0() = codet(ID_skip);

  code_blockt block;
  block.add(tmp_a);
  block.add(tmp_b);
  block.add_source_location() = code.source_location();

  code = block;
}

/// Transforms the codet stored in in \p f_code, which is a call to function
/// CProver.getCurrentThreadID:()I into a code_assignt as described by the
/// documentation of the function convert_thread_block.
///
/// The resulting code_blockt is stored in the output parameter \p code.
///
/// \param f_code: function call to CProver.getCurrentThreadID:()I
/// \param [out] code: resulting transformation
/// \param symbol_table: a symbol table
static void instrument_getCurrentThreadID(
  const code_function_callt &f_code,
  codet &code,
  symbol_tablet symbol_table)
{
  PRECONDITION(f_code.arguments().size() == 0);

  const symbolt& current_symbol =
    add_or_get_symbol(symbol_table,
      thread_id,
      thread_id,
      java_int_type(),
      from_integer(0, java_int_type()),
      true, true);

  code_assignt code_assign(f_code.lhs(), current_symbol.symbol_expr());
  code_assign.add_source_location() = f_code.source_location();

  code = code_assign;
}

/// Iterate through the symbol table to find and appropriately instrument
/// thread-blocks.
///
/// A thread block is a sequence of codet's surrounded with calls to
/// CProver.startThread:(I)V and CProver.endThread:(I)V. A thread block
/// corresponds to the body of the thread to be created. The only parameter
/// accepted by these functions is an integer used to differentiate between
/// different thread blocks. Function startThread() is transformed into a codet
/// ID_start_thread, which carries a target label in the field 'destination'.
/// Similarly endThread() is transformed into a codet ID_end_thread, which
/// marks the end of the thread body.  Both codet's will later be transformed
/// (in goto_convertt) into the goto instructions START_THREAD and END_THREAD.
///
/// Additionally the variable __CPROVER_thread_id (thread local) will store
/// the ID of the new thread. The new id is obtained by incrementing a global
/// variable __CPROVER_next_thread_id.
///
/// The ID of the thread is made accessible to the Java program by having calls
/// to the function 'CProver.getCurrentThreadID()I' replaced by the variable
/// __CPROVER_thread_id. We also perform this substitution in here. The
/// substitution that we perform here assumes that calls to
/// getCurrentThreadID() are ONLY made in the RHS of an expression.
///
/// Example:
///
/// \code
/// CProver.startThread(333)
/// ... // thread body
/// CProver.endThread(333)
/// \endcode
///
/// Is transformed into:
///
/// \code
/// codet(id=ID_start_thread, destination=__CPROVER_THREAD_ENTRY_333)
/// codet(id=ID_goto, destination=__CPROVER_THREAD_EXIT_333)
/// codet(id=ID_label, label=__CPROVER_THREAD_ENTRY_333)
/// codet(id=ID_atomic_begin)
/// __CPROVER_next_thread_id += 1;
/// __CPROVER_thread_id = __CPROVER_next_thread_id;
/// ... // thread body
/// codet(id=ID_end_thread)
/// codet(id=ID_label, label=__CPROVER_THREAD_EXIT_333)
/// \endcode
///
/// Observe that the semantics of ID_start_thread roughly corresponds to: fork
/// the current thread, continue the execution of the current thread in the
/// next line, and continue the execution of the new thread at the destination
/// field of the codet (__CPROVER_THREAD_ENTRY_333).
///
/// Note: the current version assumes that a call to startThread(n), where n is
/// an immediate integer, is in the same scope (nesting block) as a call to
/// endThread(n). Some assertion will fail during symex if this is not the case.
///
/// Note': the ID of the thread is assigned after the thread is created, this
/// creates bogus interleavings. Currently, it's not possible to
/// assign the thread ID before the creation of the thread, due to a bug in
/// symex. See https://github.com/diffblue/cbmc/issues/1630/for more details.
///
/// \param symbol_table: a symbol table
void convert_threadblock(symbol_tablet &symbol_table)
{
  using instrument_callbackt =
      std::function<void(const code_function_callt&, codet&, symbol_tablet&)>;
  using expr_replacement_mapt =
      std::unordered_map<const exprt, instrument_callbackt, irep_hash>;

  namespacet ns(symbol_table);

  for(auto entry : symbol_table)
  {
    expr_replacement_mapt expr_replacement_map;
    const symbolt &symbol = entry.second;

    // Look for code_function_callt's (without breaking sharing)
    // and insert each one into a replacement map along with an associated
    // callback that will handle their instrumentation.
    for(auto it = symbol.value.depth_begin(), itend = symbol.value.depth_end();
       it != itend; ++it)
    {
      instrument_callbackt cb;

      const exprt &expr = *it;
      if(expr.id() != ID_code)
        continue;

      const codet &code = to_code(expr);
      if(code.get_statement() != ID_function_call)
        continue;

      const code_function_callt &f_code = to_code_function_call(code);
      const std::string &f_name = expr2java(f_code.function(), ns);
      if(f_name == "org.cprover.CProver.startThread:(I)V")
        cb = std::bind(instrument_start_thread, std::placeholders::_1,
          std::placeholders::_2, std::placeholders::_3);
      else if(f_name == "org.cprover.CProver.endThread:(I)V")
        cb = std::bind(&instrument_endThread, std::placeholders::_1,
          std::placeholders::_2, std::placeholders::_3);
      else if(f_name == "org.cprover.CProver.getCurrentThreadID:()I")
        cb = std::bind(&instrument_getCurrentThreadID, std::placeholders::_1,
          std::placeholders::_2, std::placeholders::_3);

      if(cb)
        expr_replacement_map.insert({expr, cb});
    }

    if(!expr_replacement_map.empty())
    {
      symbolt &w_symbol = symbol_table.get_writeable_ref(entry.first);
      // Use expr_replacment_map to look for exprt's that need to be replaced.
      // Once found, get a non-const exprt (breaking sharing in the process) and
      // call it's associated instrumentation function.
      for(auto it = w_symbol.value.depth_begin(),
         itend = w_symbol.value.depth_end(); it != itend;)
      {
        expr_replacement_mapt::iterator m_it = expr_replacement_map.find(*it);
        if(m_it != expr_replacement_map.end())
        {
          codet &code = to_code(it.mutate());
          const code_function_callt &f_code = to_code_function_call(code);
          m_it->second(f_code, code, symbol_table);
          it.next_sibling_or_parent();

          expr_replacement_map.erase(m_it);
          if(expr_replacement_map.empty())
            break;
        }
        else
          ++it;
      }
    }
  }
}

/// Iterate through the symbol table to find and appropriately instrument
/// synchronized methods.
///
/// A synchronized method is a method with the synchronization keyword. Any
/// code inside of such methods has to be thread-safe. To this end, a call to
/// "java::java.lang.Object.monitorenter:(Ljava/lang/Object;)V" and
/// "java::java.lang.Object.monitorexit:(Ljava/lang/Object;)V" is instrumented
/// at the start and end of the method. Note, that the former is instrumented
/// at every statement where the execution can exit the method in question.
/// Specifically, out of order return statements and exceptions. The latter
/// is dealt with by instrumenting a try catch block.
///
/// Note: current version cannot handle static methods.
///
/// Note': This method requires the availability of
///   "java::java.lang.Object.monitorenter:(Ljava/lang/Object;)V" and
///   "java::java.lang.Object.monitorexit:(Ljava/lang/Object;)V".
///   (java-library-models)
///
/// \param symbol_table: a symbol table
void convert_synchronized_methods(symbol_tablet &symbol_table)
{
  for(auto entry : symbol_table)
  {
    const symbolt &symbol = entry.second;

    if(symbol.type.id() != ID_code)
        continue;
    if(symbol.value.is_nil())
        continue;
    if(symbol.type.get(ID_is_synchronized)!="1")
        continue;

    bool is_static=(symbol.type.get(ID_is_static)=="1");
    if(is_static)
      // FIXME:  handle static synchronized methods.
      continue;

    // Find the object to synchronize one
    irep_idt this_id(id2string(symbol.name)+"::this");
    exprt this_expr(this_id);

    symbol_tablet::symbolst::const_iterator it
      = symbol_table.symbols.find(this_id);

    if(it == symbol_table.symbols.end())
      // Failed to find object to synchronize on,
      continue;

    // get writeable reference and instrument the method
    symbolt &w_symbol = symbol_table.get_writeable_ref(entry.first);
    instrument_synchronized_code(
      symbol_table, w_symbol, it->second.symbol_expr());
   }
}
