#ifndef _PPC_H_
#define _PPC_H_

#if defined(_MSC_VER) || defined(__CYGWIN__)
#define DLL_EXPORT __declspec(dllexport)
#else // defined(_MSC_VER) || defined(__CYGWIN__)
#define DLL_EXPORT
#endif // defined(_MSC_VER) || defined(__CYGWIN__)

extern void genobj(const COMPILER::scope*);

extern void genfunc(const COMPILER::fundef* fun, const std::vector<COMPILER::tac*>& vc);

extern std::string func_label;

extern bool constant_object(COMPILER::usr*);

extern bool string_literal(COMPILER::usr*);

struct reg {
        enum kind { gpr, fpr };
        kind m_kind;
        int m_no;
        reg(kind kind, int no) : m_kind(kind), m_no(no) {}
};

class address {
public:
  enum mode { low, high };
  virtual void load(const reg&, mode = low) const = 0;
  virtual void store(const reg&, mode = low) const = 0;
  virtual void get(const reg&) const = 0;
  virtual ~address(){}
};

class mem : public address {
  std::string m_label;
  int m_size;
  void load_gpr(const reg&, mode) const;
  void load_fpr(const reg&) const;
  void store_gpr(const reg&, mode) const;
  void store_fpr(const reg&) const;
public:
  mem(std::string label, int size) : m_label(label), m_size(size) {}
  void load(const reg&, mode = low) const;
  void store(const reg&, mode) const;
  void get(const reg&) const;
  std::string label() const { return m_label; }
};

extern std::auto_ptr<mem> double_0x10000080000000;
extern std::auto_ptr<mem> double_0x80000000;
extern std::auto_ptr<mem> double_0x100000000;

extern bool integer_suffix(int);

class imm : public address {
  std::string m_value;
  std::pair<int,int> m_pair;
  struct bad_op {};
public:
  imm(COMPILER::usr*);
  void load(const reg&, mode) const;
  void store(const reg&, mode) const { throw bad_op(); }
  void get(const reg&) const { throw bad_op(); }
};

class stack : public address {
  int m_offset;
  int m_size;
  void load_gpr(const reg&, mode) const;
  void load_fpr(const reg&) const;
  void store_gpr(const reg&, mode) const;
  void store_fpr(const reg&) const;
public:
  stack(int offset, int size) : m_offset(offset), m_size(size) {}
  void load(const reg&, mode = low) const;
  void store(const reg&, mode = low) const;
  void get(const reg&) const;
  int offset() const { return m_offset; }
  int size() const { return m_size; }
};

extern stack* tmp_dword;
extern stack* tmp_word;

class alloced_addr : public address {
  stack m_stack;
  struct bad_op {};
public:
  alloced_addr(int offset) : m_stack(offset,4) {}
  void load(const reg&, mode) const { throw bad_op(); }
  void store(const reg&, mode) const { throw bad_op(); }
  void get(const reg& r) const { m_stack.load(r); }
  void set(const reg& r) const { m_stack.store(r); }
};

extern std::map<const COMPILER::var*, address*> address_descriptor;

enum section { rom = 1, ram, ctor, dtor };

extern void output_section(section);

inline bool is_top(COMPILER::scope* ptr) { return !ptr->m_parent; }

extern std::ostream out;

extern std::string new_label();

extern std::map<COMPILER::var*, stack*> record_param;

#ifdef CXX_GENERATOR
extern std::string scope_name(symbol_tree*);
extern std::string func_name(std::string);
extern std::string signature(const type_expr*);

class tag;
struct anonymous_table : std::map<const tag*, address*> {
  static void destroy(std::pair<const tag*, address*> pair)
  {
    delete pair.second;
  }
  void clear()
  {
    std::for_each(begin(),end(),destroy);
    using namespace std;
    map<const tag*, address*>::clear();
  }
  ~anonymous_table()
  {
    clear();
  }
};

extern anonymous_table anonymous_table;
extern std::vector<std::string> ctors;
extern std::string dtor_name;
#endif // CXX_GENERATOR

struct function_exit {
  std::string m_label;
  bool m_ref;
};

extern struct function_exit function_exit;

extern std::string new_label();

extern bool debug_flag;

extern char get_suffix(int);

class parameter {
  int* m_res;
  int m_gpr;
  int m_fpr;
  int m_offset;
  void scalar_handler(const COMPILER::usr*);
  void aggregate_handler(const COMPILER::usr*);
public:
  parameter(int*, const COMPILER::type*);
  void operator()(const COMPILER::usr*);
  static int m_hidden;
};

struct stack_layout {
  int m_fun_arg;
  int m_size;
};

extern struct stack_layout stack_layout;

struct current_varg {
  struct {
    int m_gpr;
    int m_fpr;
  } m_number;
  struct {
    int m_gpr;
    int m_fpr;
  } m_offset;
  struct {
    stack* m_gpr;
    stack* m_fpr;
  } m_counter;
  struct {
    stack* m_gpr;
    stack* m_fpr;
  } m_pointer;
};

extern struct current_varg current_varg;

extern void enter_helper(int);
extern void leave_helper();

extern void (*output3ac)(std::ostream&, const COMPILER::tac*);

#endif // _PPC_H_
