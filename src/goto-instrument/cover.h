/*******************************************************************\

Module: Coverage Instrumentation

Author: Daniel Kroening

Date: May 2016

\*******************************************************************/

/// \file
/// Coverage Instrumentation

#ifndef CPROVER_GOTO_INSTRUMENT_COVER_H
#define CPROVER_GOTO_INSTRUMENT_COVER_H

#include <goto-programs/goto_model.h>
#include <util/cmdline.h>

class message_handlert;
class messaget;

class coverage_goalst
{
public:
  static bool get_coverage_goals(
    const std::string &coverage,
    message_handlert &message_handler,
    coverage_goalst &goals,
    const irep_idt &mode);
  void add_goal(source_locationt goal);
  bool is_existing_goal(source_locationt source_loc);
  void check_existing_goals(messaget &msg);

private:
  std::map<source_locationt, bool> existing_goals;
};

enum class coverage_criteriont
{
  LOCATION, BRANCH, DECISION, CONDITION,
  PATH, MCDC, ASSERTION, COVER };

class coverage_optionst
{
public:
  coverage_optionst(
    const symbol_tablet &symbol_table,
    const std::list<std::string> &cover_functions,
    bool function_only=false,
    bool ignore_trivial=false);

  void instrument_cover_goals(
    goto_functionst &,
    message_handlert &message_handler);

  void instrument_cover_goals(
    goto_functionst &,
    message_handlert &message_handler,
    coverage_goalst &,
    const std::string &cover_inclue_pattern="");

  void instrument_cover_goals(
    goto_programt &,
    message_handlert &message_handler,
    coverage_goalst &goals);

  void instrument_cover_goals(
    goto_programt &,
    message_handlert &message_handler);

  void set_criterion(coverage_criteriont criterion)
  {
    this->criterion=criterion;
  }

private:
  const symbol_tablet &symbol_table;
  std::set<std::string> functions;
  coverage_criteriont criterion;
  bool function_only;
  bool ignore_trivial;
};

bool consider_goals(
  const goto_programt &,
  coverage_goalst &);

bool instrument_cover_goals(
  const cmdlinet &,
  const symbol_tablet &,
  goto_functionst &,
  message_handlert &);

bool instrument_cover_goals(
  const cmdlinet &,
  goto_modelt &,
  message_handlert &);

#endif // CPROVER_GOTO_INSTRUMENT_COVER_H
