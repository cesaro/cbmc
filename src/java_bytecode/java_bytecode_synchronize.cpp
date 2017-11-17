/*******************************************************************\

Module:

Author: Dario Cattaruzza

\*******************************************************************/

#include "java_bytecode_synchronize.h"

  // Meant to be a visitor class. Currently the constructor
  // performs the visits.
  java_bytecode_synchronizet::java_bytecode_synchronizet(
    symbol_tablet& _symbol_table) :
  symbol_table(_symbol_table)
  {
    forall_symbols(symbol, symbol_table.symbols)
    {
      if((symbol->second.type.id()==ID_code)
        && (symbol->second.value.is_not_nil()))
      {
        symbolt &w_symbol=*symbol_table.get_writeable(symbol->second.name);
        visit_symbol(w_symbol);
      }
    }
  }

  /// This function will wrap the code inside a synchronized block.
  /// This is achieved by placing a monitorenter at the beginning and a
  /// monitorexit at every return. Additionally the whole block is wrapped in
  /// a try-finally in order to recover the monitorexit in case of exceptions
  void java_bytecode_synchronizet::visit_symbol(symbolt &symbol)
  {
    if(symbol.type.get(ID_is_synchronized)!="1")
      return;
    bool is_static=(symbol.type.get(ID_is_static)=="1");
    codet &code=to_code(symbol.value);

    // Find object to lock/synchronize on
    irep_idt this_id(id2string(symbol.name)+"::this");
    exprt this_expr(this_id);
    if(is_static)
    {
      // FIXME: place code to get class here
      // getClass doesnot have a singleton at the moment
      // to retrieve from the symbol table.
    }
    symbol_tablet::symbolst::const_iterator it
        =symbol_table.symbols.find(this_id);
    if(it!=symbol_table.symbols.end())
    {
      this_expr=it->second.symbol_expr();
    }

    // get interposition code for the monitor lock/unlock
    codet monitorenter=get_monitor_call(true, this_expr);
    codet monitorexit=get_monitor_call(false, this_expr);

    // Create a unique catch label and empty throw type (ie any)
    // and catch-push them at the beginning of the code (ie begin try).
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
    symbol_exprt catch_var=
      tmp_variable(
        id2string(symbol.name),
        "caught_synchronized_exception",
        java_reference_type(symbol_typet("java::java.lang.Throwable")));
    code_landingpadt catch_statement(catch_var);
    codet catch_instruction=catch_statement;
    code_labelt catch_label(handler, code_blockt());
    code_blockt &catch_block=to_code_block(catch_label.code());
    catch_block.add(catch_instruction);
    catch_block.add(monitorexit);

    // rethrow exception
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
    code=try_block;
  }

  /// Creates a temporary variable (used to record the caught exception)
  symbol_exprt java_bytecode_synchronizet::tmp_variable(
    const std::string &prefix,
    const std::string &name,
    const typet &type)
  {
    irep_idt base_name=name+"_tmp";
    irep_idt identifier=prefix+"::"+id2string(base_name);

    auxiliary_symbolt tmp_symbol;
    tmp_symbol.base_name=base_name;
    tmp_symbol.is_static_lifetime=false;
    tmp_symbol.mode=ID_java;
    tmp_symbol.name=identifier;
    tmp_symbol.type=type;
    symbol_table.add(tmp_symbol);

    symbol_exprt result(identifier, type);
    result.set(ID_C_base_name, base_name);
    return result;
  }

  /// Retrieves a monitorenter/monitorexit call expr for the given object.
  /// \param is_enter Indicates whether we are creating a monitorenter or exit.
  /// \param object An expression givin the object in which we need to perform a
  ///   monitorenter/exit operation.
  codet java_bytecode_synchronizet::get_monitor_call(
    bool is_enter,
    exprt &object)
  {
    code_typet type;
    type.return_type()=void_typet();
    type.parameters().resize(1);
    type.parameters()[0].type()=java_reference_type(void_typet());
    std::string symbol;
    if(is_enter)
      symbol="java::java.lang.Object.monitorenter:(Ljava/lang/Object;)V";
    else
      symbol="java::java.lang.Object.monitorexit:(Ljava/lang/Object;)V";
    symbol_tablet::symbolst::const_iterator it
      =symbol_table.symbols.find(symbol);

    // If the functions for monitorenter and monitorexit don't exist in the
    // otherwise given jars, we implement them as skips because cbmc would
    // crash, since it cannot find the function in the symbol table (function
    // called but undefined symbol).
    // FIXME: a better way to do this would be to have a final pass that
    // checks every function called in the codet and inserts missing symbols.
    if(it==symbol_table.symbols.end())
      return code_skipt();

    // Otherwise we create a function call
    code_function_callt call;
    call.function()=symbol_exprt(symbol, type);
    call.lhs().make_nil();
    call.arguments().push_back(object);
    return call;
  }

  /// Introduces a monitorexit before every return recursively
  /// \param code current element to check
  /// \param monitorexit codet to insert before the return.
  void java_bytecode_synchronizet::monitor_exits(
    codet &code,
    codet &monitorexit)
  {
    const irep_idt &statement=code.get_statement();
    if(statement==ID_return)
    {
      // Relace the return with a monitor exit plus return block
      code_blockt return_block;
      return_block.add(monitorexit);
      return_block.move_to_operands(code);
      code=return_block;
    }
    else if((statement==ID_label)
      || (statement==ID_block)
      || (statement==ID_decl_block))
    {
      // If label or block found, explore the code inside the block
      Forall_operands(it, code)
      {
        codet &sub_code=to_code(*it);
        monitor_exits(sub_code, monitorexit);
      }
    }
  }
