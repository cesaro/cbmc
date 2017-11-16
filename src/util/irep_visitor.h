
/*
 * FIXME
 * Incomplete experiment to implement visitors for ireps, exprt, codet, and
 * symbol_tablet
 *
 * - The experiment uses static polymorphism to inject methods in the visitor
 * - Thanks to it the compiler will be able to optimize the large if in the
 *   visit() method, as it can see that the default implementations just return
 *   true and do nothing.
 *
 */

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// 1st experiment

template<class Derived>
class ExprVisitor
{
public:
   template<typename T>
   bool visit_type (T i)
   {
      printf ("ExprVisitor.visit: type %zu\n", sizeof (T));
      return true;
   }

   void visit (int i)
   {
      char c = 'C';
      float f = 3.1415;
      static_cast<Derived*>(this)->visit_type (i);
      static_cast<Derived*>(this)->visit_type (c);
      static_cast<Derived*>(this)->visit_type (f);
   }

};

class CountVisitor : public ExprVisitor<CountVisitor>
{
public:

   int count = 0;

   template<class X>
   void method (X x)
   {
   }


   //template<>
   //bool visit_type<short> (int i)
   //{
   //   printf ("CountVisitor.visit<short>!!\n");
   //   return false;
   //}
};

template<>
template<>
bool ExprVisitor<CountVisitor>::visit_type<int> (int i)
{
   //count++;
   return false;
}

void test32 ()
{
   CountVisitor v;

   v.visit (123);
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// 2nd experiment, probably the one I should use

template<class T>
class ExprVisitor2;

class Expr2
{
public:
   int i;
};

template<class T>
class ExprVisitor2 : public T
{

   // FIXME add a constructor that constructs T!

   template<typename C>
   bool visitt (C &e)
   {
      printf ("default\n");
      return true;
   }

public:
   void visit (Expr2 &e)
   {
      float f = 3.1415;
      int i = 123;
      char c = 'C';
      const char *s = "hello world";
      visitt (f);
      visitt (i);
      visitt (c);
      visitt (s);
   }
};

struct CountVisitor2
{
   int count;
};

template<>
template<>
bool ExprVisitor2<CountVisitor2>::visitt<char> (char &c)
{
   printf ("count visitor: char\n");
   count++;
   return false;
}

template<>
template<>
bool ExprVisitor2<CountVisitor2>::visitt<float> (float &f)
{
   printf ("count visitor: float\n");
   count++;
   return false;
}

void test33 ()
{
   Expr2 e;

   ExprVisitor2<CountVisitor2> v;
   v.count = 0;
   v.visit (e);
   printf ("end: count %d\n", v.count);
}

int main (int argc, char ** argv)
{
   //main25 (argc, argv);
   test32 ();
   return 0;
}


// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx


enum irep_visitor_searcht
{
  DFS,
  BFS
};

template<irep_visitor_searcht kind=irep_visitor_searcht::DFS,
  bool do_comments=false>
class irep_visitort
{
public:
  void visit(irept &i)
  {
    if(kind==irep_visitor_searcht::DFS)
      visit_dfs(i);
    else
      visit_bfs(i);
  }

protected:
  void visit_dfs (irep &i)
  {
    std::stack<irept *> stack;

    stack.push(&i);

    while(!stack.empty())
    {
      irep &ii=*stack.top();
      stack.pop();

      visit_irep(ii);

      for(auto &it : ii.named_sub)
        stack.push(&it->second);
      for(auto &it : ii.sub)
        stack.push(&(*it));
      if(do_comments)
        for(auto &it : ii.comments)
          stack.push(&it->second);
    }
  }

  void visit_bfs (irep &i);
  void visit_irep (irep &i)
  {}
};

template<irep_visitor_searcht kind=irep_visitor_searcht::DFS,
  bool do_comments=false>
class const_irep_visitort
{
public:
  void visit(const irept &i)
  {
    if(kind==irep_visitor_searcht::DFS)
      visit_dfs(i);
    else
      visit_bfs(i);
  }

protected:
  void visit_dfs (const irep &i);
  void visit_bfs (const irep &i);
  void visit_irep (const irep &i)
  {}
};

template<class Derived, irep_visitor_searcht kind=irep_visitor_searcht::DFS>
class expr_visitort : public irep_visitor<kind,false>
{

  void visit(exprt &e)
  {
    if (e.id() == ID_declaration)
      static_cast<Derived*>(this)->
  }

  typename<T>
  bool visit_expr (T &e) { return true; }

	// FIXME: these methods can be removed using the two experiments above!
  bool visit_ansi_c_declaration (ansi_c_declarationt &e) { return true; }
  bool visit_ansi_c_declarator (ansi_c_declaratort &e) { return true; }
  bool visit_array_expr (array_exprt &e) { return true; }
  bool visit_binary_expr (binary_exprt &e) { return true; }
  // FIXME .. all classes that inherit from binary_exprt instead of
  // binary_exprt?
  bool visit_byte_update_expr (byte_update_exprt &e) { return true; }
  bool visit_code (codet &e) { return true; }
  bool visit_concatenation_expr (concatenation_exprt &e) { return true; }
  bool visit_constant_expr (constant_exprt &e) { return true; }
  // FIXME all classe that inherit from constant_exprt instead?
  bool visit_cpp_declaration (cpp_declarationt &e) { return true; }
  // ... see doxygen
};
