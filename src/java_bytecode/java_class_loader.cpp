/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include "java_class_loader.h"

#include <stack>
#include <map>
#include <fstream>

#include <util/suffix.h>
#include <util/prefix.h>
#include <util/config.h>

#include "java_bytecode_parser.h"
#include "jar_file.h"

#include "java_core_models.inc"

/// This variable stores the data of the file core-models.jar. The macro
/// JAVA_CORE_MODELS_SIZE is defined in the header java_core_models.inc, which
/// gets generated at compile time by running a small utility (converter.cpp) on
/// actual .jar file. The number of bytes in the variable is
/// JAVA_CORE_MODELS_SIZE, another macro defined in java_core_models.inc.
unsigned char java_core_models[] = { JAVA_CORE_MODELS_DATA };

java_bytecode_parse_treet &java_class_loadert::operator()(
  const irep_idt &class_name)
{
  std::stack<irep_idt> queue;

  queue.push(class_name);

  // Add the necessary classes in reverse order
  // since this is a FILO

  // add built-in-functions
  queue.push("org.cprover.CProver");

  // Require java.lang.Throwable as the catch-type used for
  // universal exception handlers:
  queue.push("java.lang.Throwable");

  // add java.lang.Class
  queue.push("java.lang.Class");

  // java.lang.String
  queue.push("java.lang.String");

  // Always require java.lang.Object, as it is the base of
  // internal classes such as array types.
  queue.push("java.lang.Object");

  java_class_loader_limitt class_loader_limit(
    get_message_handler(), java_cp_include_files);

  while(!queue.empty())
  {
    irep_idt c=queue.top();
    queue.pop();

    // do we have the class already?
    if(class_map.find(c)!=class_map.end())
      continue; // got it already

    debug() << "Reading class " << c << eom;

    java_bytecode_parse_treet &parse_tree=
      get_parse_tree(class_loader_limit, c);

    // add any dependencies to queue
    for(java_bytecode_parse_treet::class_refst::const_iterator
        it=parse_tree.class_refs.begin();
        it!=parse_tree.class_refs.end();
        it++)
      queue.push(*it);
  }

  return class_map[class_name];
}

void java_class_loadert::set_java_cp_include_files(
  std::string &_java_cp_include_files)
{
  java_cp_include_files=_java_cp_include_files;
}

bool java_class_loadert::get_class_file(
  java_class_loader_limitt &class_loader_limit,
  const irep_idt &class_name,
  const std::string &jar_file,
  java_bytecode_parse_treet &parse_tree)
{
  const auto &jm=jar_map[jar_file];

  auto jm_it=jm.entries.find(class_name);

  if(jm_it!=jm.entries.end())
  {
    debug() << "Getting class `" << class_name << "' from JAR "
            << jar_file << eom;

    std::string data=jar_pool(class_loader_limit, jar_file)
      .get_entry(jm_it->second.class_file_name);

    std::istringstream istream(data);

    java_bytecode_parse(
      istream,
      parse_tree,
      get_message_handler());

    return true;
  }
  return false;
}

bool java_class_loadert::get_internal_class_file(
  java_class_loader_limitt &class_loader_limit,
  const irep_idt &class_name,
  java_bytecode_parse_treet &parse_tree)
{
  // Add internal jar file. The name is used to load it once only and
  // reference it later.
  std::string core_models="core-models.jar";
  jar_pool(class_loader_limit,
           core_models,
           java_core_models,
           JAVA_CORE_MODELS_SIZE);

  // This does not read from the jar file but from the jar_filet object
  // as we've just created it
  read_jar_file(class_loader_limit, core_models);
  return
    get_class_file(class_loader_limit, class_name, core_models, parse_tree);
}

java_bytecode_parse_treet &java_class_loadert::get_parse_tree(
  java_class_loader_limitt &class_loader_limit,
  const irep_idt &class_name)
{
  java_bytecode_parse_treet &parse_tree=class_map[class_name];

  // First check given JAR files
  for(const auto &jf : jar_files)
  {
    read_jar_file(class_loader_limit, jf);
    if(get_class_file(class_loader_limit, class_name, jf, parse_tree))
      return parse_tree;
  }

  if(use_core_models && prefer_core_models)
  {
    if(get_internal_class_file(class_loader_limit, class_name, parse_tree))
      return parse_tree;
  }

  // See if we can find it in the class path
  for(const auto &cp : config.java.classpath)
  {
    // in a JAR?
    if(has_suffix(cp, ".jar"))
    {
      read_jar_file(class_loader_limit, cp);
      if(get_class_file(class_loader_limit, class_name, cp, parse_tree))
        return parse_tree;
    }
    else
    {
      // in a given directory?
      std::string full_path=
        #ifdef _WIN32
        cp+'\\'+class_name_to_file(class_name);
        #else
        cp+'/'+class_name_to_file(class_name);
        #endif

      // full class path starts with './'
      if(class_loader_limit.load_class_file(full_path.substr(2)) &&
         std::ifstream(full_path))
      {
        if(!java_bytecode_parse(
             full_path,
             parse_tree,
             get_message_handler()))
          return parse_tree;
      }
    }
  }

  if(use_core_models && !prefer_core_models)
  {
    if(get_internal_class_file(class_loader_limit, class_name, parse_tree))
      return parse_tree;
  }

  // not found
  warning() << "failed to load class `" << class_name << '\'' << eom;
  parse_tree.parsed_class.name=class_name;
  return parse_tree;
}

void java_class_loadert::load_entire_jar(
  java_class_loader_limitt &class_loader_limit,
  const std::string &file)
{
  read_jar_file(class_loader_limit, file);

  const auto &jm=jar_map[file];

  jar_files.push_front(file);

  for(const auto &e : jm.entries)
    operator()(e.first);

  jar_files.pop_front();
}

void java_class_loadert::read_jar_file(
  java_class_loader_limitt &class_loader_limit,
  const irep_idt &file)
{
  // done already?
  if(jar_map.find(file)!=jar_map.end())
    return;

  std::vector<std::string> filenames;
  try
  {
    filenames=this->jar_pool(class_loader_limit, id2string(file)).filenames();
  }
  catch(const std::runtime_error &)
  {
    error() << "failed to open JAR file `" << file << "'" << eom;
    return;
  }
  debug() << "adding JAR file `" << file << "'" << eom;

  // create a new entry in the map and initialize using the list of file names
  // that were retained in the jar_filet by the class_loader_limit filter
  auto &jm=jar_map[file];
  for(auto &file_name : filenames)
  {
    // does it end on .class?
    if(has_suffix(file_name, ".class"))
    {
      debug() << "read class file " << file_name << " from " << file << eom;
      irep_idt class_name=file_to_class_name(file_name);

      // record
      jm.entries[class_name].class_file_name=file_name;
    }
  }
}

std::string java_class_loadert::file_to_class_name(const std::string &file)
{
  std::string result=file;

  // Strip .class. Note that the Java class loader would
  // not do that.
  if(has_suffix(result, ".class"))
    result.resize(result.size()-6);

  // Strip a "./" prefix. Note that the Java class loader
  // would not do that.
  #ifdef _WIN32
  while(has_prefix(result, ".\\"))
    result=std::string(result, 2, std::string::npos);
  #else
  while(has_prefix(result, "./"))
    result=std::string(result, 2, std::string::npos);
  #endif

  // slash to dot
  for(std::string::iterator it=result.begin(); it!=result.end(); it++)
    if(*it=='/')
      *it='.';

  return result;
}

std::string java_class_loadert::class_name_to_file(const irep_idt &class_name)
{
  std::string result=id2string(class_name);

  // dots (package name separators) to slash, depending on OS
  for(std::string::iterator it=result.begin(); it!=result.end(); it++)
    if(*it=='.')
    {
      #ifdef _WIN32
      *it='\\';
      #else
      *it='/';
      #endif
    }

  // add .class suffix
  result+=".class";

  return result;
}

jar_filet &java_class_loadert::jar_pool(
  java_class_loader_limitt &class_loader_limit,
  const std::string &file_name)
{
  const auto it=m_archives.find(file_name);
  if(it==m_archives.end())
  {
    // VS: Can't construct in place
    auto file=jar_filet(class_loader_limit, file_name);
    return m_archives.emplace(file_name, std::move(file)).first->second;
  }
  else
    return it->second;
}

jar_filet &java_class_loadert::jar_pool(
  java_class_loader_limitt &class_loader_limit,
  const std::string &buffer_name,
  const void *pMem,
  size_t size)
{
  const auto it=m_archives.find(buffer_name);
  if(it==m_archives.end())
  {
    // VS: Can't construct in place
    auto file=jar_filet(class_loader_limit, pMem, size);
    return m_archives.emplace(buffer_name, std::move(file)).first->second;
  }
  else
    return it->second;
}
