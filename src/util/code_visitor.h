/*******************************************************************\

Module: Misc Utilities

Author: Diffblue

\*******************************************************************/

/// \file
/// code_visitor.h
#ifndef CPROVER_UTIL_CODE_VISITOR_H
#define CPROVER_UTIL_CODE_VISITOR_H

#include <cstdio>
#include <map>
#include <functional>
#include "symbol_table.h"
#include "std_code.h"

class code_visitort:public expr_visitort
{
public:
  typedef std::function<void(codet&)> callbackt;
  typedef std::map<irep_idt, callbackt> callmapt;

  explicit code_visitort(callmapt& cv) : call_map(cv) {}

  void visit_symbols(symbol_tablet &symbol_table)
  {
    forall_symbols(symbol, symbol_table.symbols)
    {
      symbolt &w_symbol=*symbol_table.get_writeable(symbol->second.name);
      w_symbol.value.visit(*this);
    }
  }

  void operator()(exprt &expr) final
  {
    if(expr.id()!=ID_code || !expr.is_not_nil())
      return;
    codet &code=to_code(expr);
    const irep_idt &statement=code.get_statement();
    auto it=call_map.find(statement);
    if(it!=call_map.end())
      it->second(code);
  };

private:
  callmapt &call_map;
};

#endif // CPROVER_UTIL_CODE_VISITOR_H
