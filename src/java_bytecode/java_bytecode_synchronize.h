/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#ifndef CPROVER_JAVA_BYTECODE_JAVA_BYTECODE_SYNCHRONIZE_H
#define CPROVER_JAVA_BYTECODE_JAVA_BYTECODE_SYNCHRONIZE_H

#include <util/symbol_table.h>
#include "java_types.h"
#include <util/std_code.h>
#include <util/std_expr.h>

class java_bytecode_synchronizet
{
private:
  symbol_tablet &symbol_table;
public:
  java_bytecode_synchronizet(symbol_tablet& _symbol_table);

  /**
   * This function will wrap the code inside a synchronized block
   * This is achieved by placing a monitorenter at the beginning and a monitorexit
   * at every return. Additionally the whole block is wrapped in a try-finally in
   * order to recover the monitorexit in case of exceptions
   */
  virtual void visit_symbol(symbolt &symbol);

protected:
  /**
   * Creates a temporary variable
   */
  symbol_exprt tmp_variable(const std::string &prefix,
    const std::string &name,
    const typet &type);

  /**
   * Retrieves a monitorenter/monitorexit call expr for the given object
   */
  codet get_monitor_call(bool is_enter, exprt &object);

  /**
   * Introduces a monitorexit before every return
   */
  void monitor_exits(codet &code,codet &monitorexit);
};

#endif // CPROVER_JAVA_BYTECODE_JAVA_BYTECODE_SYNCHRONIZE_H
