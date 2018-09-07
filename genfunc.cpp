#include "stdafx.h"
#include "c_core.h"

#define COMPILER c_compiler

#include "ppc.h"
#include "gencode.h"

void enter(const COMPILER::fundef* func, const std::vector<COMPILER::tac*>& v3ac);
void leave();

#ifdef CXX_GENERATOR
std::vector<std::string> ctors;
std::string dtor_name;
struct anonymous_table anonymous_table;
#endif // CXX_GENERATOR

std::string func_label;

void genfunc(const COMPILER::fundef* func, const std::vector<COMPILER::tac*>& v3ac)
{
  using namespace std;
  using namespace COMPILER;
  output_section(rom);
#ifndef CXX_GENERATOR
  func_label = func->m_usr->m_name;
#else // CXX_GENERATOR
  func_label = scope_name(func->m_scope);
  func_label += func_name(func->m_name);
  if ( !func->m_csymbol )
        func_label += signature(func->m_type);
#endif // CXX_GENERATOR
  usr::flag flag = func->m_usr->m_flag;
  if ( !(flag & usr::STATIC) )
    out << '\t' << ".global" << '\t' << func_label << '\n';
  out << '\t' << ".align" << '\t' << 4 << '\n';
  out << func_label << ":\n";
  enter(func,v3ac);
  function_exit.m_label = new_label();
  function_exit.m_ref = false;
  if ( !v3ac.empty() )
    accumulate(v3ac.begin(), v3ac.end(), 0, gencode(v3ac));
  leave();
#ifdef CXX_GENERATOR
  switch ( func->m_initialize_or_destruct ){
  case _initialize: ctors.push_back(name); break;
  case _destruct: dtor_name = name; break;
  }
#endif // CXX_GENERATOR
}

int sched_stack(const COMPILER::fundef* func, const std::vector<COMPILER::tac*>&);

class save_param {
  int m_gpr;
  int m_fpr;
  int m_offset;
  void save_gpr(const stack*);
  void save_fpr(const stack*);
  void decide_real(const COMPILER::usr*);
  void decide_notreal(const COMPILER::usr*);
  void decide_aggregate(const COMPILER::usr*);
public:
  save_param();
  void operator()(const COMPILER::usr*);
};

void save_if_varg();

void enter_helper(int n)
{
  out << '\t' << "stwu 1," << -n << "(1)" << '\n';
  out << '\t' << "mflr 0" << '\n';
  out << '\t' << "stw 31," << n-4 << "(1)" << '\n';
  out << '\t' << "stw 0," << n+4 << "(1)" << '\n';
  out << '\t' << "mr 31,1" << '\n';
}

void enter(const COMPILER::fundef* func, const std::vector<COMPILER::tac*>& v3ac)
{
  using namespace std;
  using namespace COMPILER;
  if ( debug_flag )
    out << '\t' << "/* enter */" << '\n';

  int n = sched_stack(func,v3ac);
  enter_helper(n);

  param_scope* param = func->m_param;
  const vector<usr*>& vec = param->m_order;
  for_each(vec.begin(),vec.end(),save_param());
  save_if_varg();
}

void clear_address_descriptor()
{
  using namespace std;
  using namespace COMPILER;
  typedef map<const var*, address*>::iterator IT;
  for ( IT p = address_descriptor.begin() ; p != address_descriptor.end() ; ){
    const var* entry = p->first;
    if ( is_top(entry->m_scope) )
      ++p;
    else {
      IT q = p++;
      delete q->second;
          address_descriptor.erase(q);
    }
  }
}

inline void destroy(const std::pair<const COMPILER::var*, stack*>& x)
{
  delete x.second;
}

void clear_record_param()
{
        using namespace std;
        for_each(record_param.begin(),record_param.end(),destroy);
        record_param.clear();
}

void clear_current_varg();

void leave_helper()
{
  using namespace std;
  pair<int,int> tmp(11,0);
  out << '\t' << "lwz " << tmp.first << ", 0(1)" << '\n';
  out << '\t' << "lwz " << tmp.second << ", 4(" << tmp.first << ')' << '\n';
  out << '\t' << "mtlr " << tmp.second << '\n';
  out << '\t' << "lwz 31,-4(" << tmp.first << ')' << '\n';
  out << '\t' << "mr 1," << tmp.first << '\n';
  out << '\t' << "blr" << '\n';
}

void leave()
{
  if ( debug_flag )
    out << '\t' << "/* leave */" << '\n';
  if ( function_exit.m_ref )
    out << function_exit.m_label << ":\n";
  leave_helper();

  clear_record_param();
  clear_address_descriptor();
#ifdef CXX_GENERATOR
  anonymous_table.clear();
#endif // CXX_GENERATOR
  clear_current_varg();
}

int fun_arg(const std::vector<COMPILER::tac*>&);

struct stack_layout stack_layout;

int local_variable(const COMPILER::fundef* func, int);

int sched_stack(const COMPILER::fundef* func, const std::vector<COMPILER::tac*>& v3ac)
{
  int n = fun_arg(v3ac);
  stack_layout.m_fun_arg = n;
  n = local_variable(func,n);
  stack_layout.m_size = n;
  return n;
}

class arg_count {
  int* m_res;
  int m_curr;
public:
  arg_count(int* res) : m_res(res), m_curr(0) {}
  void operator()(const COMPILER::tac*);
};

int param_space_aggregate(int, COMPILER::tac*);

int fun_arg(const std::vector<COMPILER::tac*>& v3ac)
{
  using namespace std;
  int n = 0;
  for_each(v3ac.begin(),v3ac.end(),arg_count(&n));
  n = (n < 32 ? 0 : n - 32) + 8;
  return accumulate(v3ac.begin(),v3ac.end(),n,param_space_aggregate);
}

inline bool cmp_id(const COMPILER::tac* ptr, COMPILER::tac::id_t id) { return ptr->id == id;  }

void arg_count::operator()(const COMPILER::tac* ptr)
{
        using namespace std;
        using namespace COMPILER;
        if (cmp_id(ptr, tac::PARAM)) {
                const var* entry = ptr->y;
                const type* T = entry->m_type;
                T = T->promotion();
                int size = T->size();
                if (T->scalar())
                        m_curr += size;
                else
                        m_curr += 4;
        }
        else if ( cmp_id(ptr, tac::CALL) ){
                *m_res = max(*m_res,m_curr);
                m_curr = 0;
        }
}

inline int align(int offset, int size)
{
        if (int n = offset % size)
                return offset + size - n;
        else
                return offset;
}

int param_space_aggregate(int offset, COMPILER::tac* ptr)
{
        using namespace std;
        using namespace COMPILER;
        if (!cmp_id(ptr, tac::PARAM))
                return offset;
        var* entry = ptr->y;
        const type* T = entry->m_type;
        if (T->scalar())
                return offset;

        offset = align(offset, 16);
        int size = T->size();
        record_param[entry] = new ::stack(offset, size);
        return offset + size;
}

class recursive_locvar {
  int* m_res;
  static int add1(int offset, const std::pair<std::string, std::vector<COMPILER::usr*> >&);
  static int add2(int offset, const COMPILER::usr* u);
  static int add3(int offset, const COMPILER::var* v);
public:
  recursive_locvar(int* p) : m_res(p) {}
  void operator()(const COMPILER::scope*);
};

int decide_if_varg(int, const COMPILER::type*, const std::vector<COMPILER::usr*>&);

int local_variable(const COMPILER::fundef* func, int n)
{
  using namespace std;
  using namespace COMPILER;
  param_scope* param = func->m_param;
  assert(param->m_children.size() == 1);
  scope* tree = param->m_children[0];

  recursive_locvar recursive_locvar(&n);
  recursive_locvar(tree);

  n = align(n,8);
  tmp_dword = new ::stack(n,8);
  n += 8;
  tmp_word = new ::stack(n,4);
  n += 4;

  const vector<usr*>& vec = param->m_order;
  const type* T = func->m_usr->m_type;
  for_each(vec.begin(),vec.end(),parameter(&n,T));
  n = decide_if_varg(n,T,vec);
  return n + 4;
}

void recursive_locvar::operator()(const COMPILER::scope* tree)
{
  using namespace std;
  using namespace COMPILER;
  const map<string, vector<usr*> >& usrs = tree->m_usrs;
  *m_res = accumulate(usrs.begin(),usrs.end(),*m_res,add1);
  assert(tree->m_id == scope::BLOCK);
  const block* b = static_cast<const block*>(tree);
  const vector<var*>& vars = b->m_vars;
  *m_res = accumulate(vars.begin(), vars.end(), *m_res, add3);
  const vector<scope*>& children = tree->m_children;
  for_each(children.begin(),children.end(),recursive_locvar(m_res));
}

int recursive_locvar::add1(int offset, const std::pair<std::string, std::vector<COMPILER::usr*> >& x)
{
        using namespace std;
        using namespace COMPILER;
        const vector<usr*>& vec = x.second;
        return accumulate(vec.begin(), vec.end(), offset, add2);
}

int recursive_locvar::add2(int offset, const COMPILER::usr* u)
{
        using namespace std;
        using namespace COMPILER;

        usr::flag flag = u->m_flag;

        map<const var*, address*>::const_iterator p = address_descriptor.find(u);
        if (p != address_descriptor.end()) {
                assert(flag & usr::STATIC);
                return offset;
        }

        if (flag & usr::TYPEDEF)
                return offset;


        usr::flag mask = usr::flag(usr::EXTERN | usr::FUNCTION);
        if (flag & mask)
                return offset;

        if (u->isconstant())
                return offset;

        const type* T = u->m_type;
        if (T->m_id == type::VARRAY) {
                offset = align(offset, 4);
                address_descriptor[u] = new alloced_addr(offset);
                return offset + 4;
        }

        offset = align(offset,T->align());
        int size = T->size();
        address_descriptor[u] = new ::stack(offset,size);
        return offset + size;
}

int recursive_locvar::add3(int offset, const COMPILER::var* v)
{
        using namespace std;
        using namespace COMPILER;
        const type* T = v->m_type;
        int size = T->size();
        offset = align(offset, T->align());
        address_descriptor[v] = new ::stack(-offset, size);
        return offset + size;
}

int parameter::m_hidden;

parameter::parameter(int* p, const COMPILER::type* T)
  : m_res(p), m_gpr(3), m_fpr(1), m_offset(0)
{
        using namespace std;
        using namespace COMPILER;
        assert(T->m_id == type::FUNC);
        typedef const func_type FUNC;
        FUNC* ft = static_cast<FUNC*>(T);
        T = ft->return_type();
        T = T->unqualified();

        if (T->m_id == type::RECORD) {
                m_hidden = *m_res;
                *m_res += 4;
                ++m_gpr;
        }
        else
                m_hidden = -1;
}

void parameter::operator()(const COMPILER::usr* entry)
{
  using namespace COMPILER;
  const type* T = entry->m_type;
  T->scalar() ? scalar_handler(entry) : aggregate_handler(entry);
}

void parameter::scalar_handler(const COMPILER::usr* entry)
{
        using namespace std;
        using namespace COMPILER;
        const type* T = entry->m_type;
        if ( T->real() ? m_fpr > 8 : m_gpr > 10 )
                return;
        int unpromed = T->size();
        T = T->promotion();
        int size = T->size();
        int offset = *m_res + size - unpromed;
        address_descriptor[entry] = new ::stack(offset,unpromed);
        *m_res += size;
        if ( T->real() )
                ++m_fpr;
        else if ( size == 4 )
                ++m_gpr;
        else
                m_gpr += ((m_gpr & 1) ? 2 : 3);
}

void parameter::aggregate_handler(const COMPILER::usr* entry)
{
  if ( m_gpr > 10 )
    return;
  int offset = *m_res;
  address_descriptor[entry] = new stack(offset,-1);
  *m_res += 4;
  ++m_gpr;
}

save_param::save_param() : m_gpr(3), m_fpr(1), m_offset(0)
{
  if ( parameter::m_hidden < 0 )
    return;
  out << '\t' << "stw " << m_gpr++ << ',' << parameter::m_hidden << "(31)\n";
}

void save_param::operator()(const COMPILER::usr* entry)
{
        using namespace std;
        using namespace COMPILER;
        const type* T = entry->m_type;
        if ( T->real() ){
                if ( m_fpr > 8 )
                        return decide_real(entry);
        }
        else {
                if ( m_gpr > 10 )
                        return T->scalar() ? decide_notreal(entry) : decide_aggregate(entry);
        }

        map<const var*, address*>::const_iterator p = address_descriptor.find(entry);
        assert(p != address_descriptor.end());
        address* ad = p->second;
        const ::stack* st = static_cast<const ::stack*>(ad);
        T->real() ? save_fpr(st) : save_gpr(st);
}

void save_param::save_gpr(const stack* st)
{
  int offset = st->offset();
  int size = st->size();
  if ( size < 0 )
    size = 4;
  else if ( size < 4 )
    offset -= 4 - size;
  else if ( size == 8 ){
    if ( !(m_gpr & 1) )
      ++m_gpr;
  }

  out << '\t' << "stw " << m_gpr++ << ',' << offset << "(31)" << '\n';
  if ( size != 8 )
    return;
  offset += 4;
  out << '\t' << "stw " << m_gpr++ << ',' << offset << "(31)" << '\n';
}

void save_param::save_fpr(const stack* st)
{
  int offset = st->offset();
  int size = st->size();
  char suffix = size == 4 ? 's' : 'd';
  out << '\t' << "stf" << suffix << ' ' << m_fpr++ << ',';
  out << offset << "(31)" << '\n';
}

void save_param::decide_notreal(const COMPILER::usr* entry)
{
        using namespace std;
        using namespace COMPILER;
        if (!m_offset)
                m_offset = stack_layout.m_size + 8;
        const type* T = entry->m_type;
        int unpromed = T->size();
        T = T->promotion();
        int size = T->size();
        m_offset = align(m_offset, T->align());
        address_descriptor[entry] = new ::stack(m_offset + size - unpromed, unpromed);
        m_offset += size;
}

void save_param::decide_aggregate(const COMPILER::usr* entry)
{
        using namespace std;
        using namespace COMPILER;
        if (!m_offset)
                m_offset = stack_layout.m_size + 8;
        address_descriptor[entry] = new ::stack(m_offset, -1);
        m_offset += 4;
}

void save_param::decide_real(const COMPILER::usr* entry)
{
        using namespace std;
        using namespace COMPILER;
        if ( !m_offset )
            m_offset = stack_layout.m_size + 8;
        const type* T = entry->m_type;
        int size = T->size();
        m_offset = align(m_offset, T->align());
        address_descriptor[entry] = new ::stack(m_offset, size);
        m_offset += size;
}

bool take_varg(const COMPILER::type* T)
{
        using namespace std;
        using namespace COMPILER;
        assert(T->m_id == type::FUNC);
        typedef const func_type FUNC;
        FUNC* ft = static_cast<FUNC*>(T);
        const vector<const type*>& param = ft->param();
        if (param.empty())
                return false;
        T = param.back();
        return T->m_id == type::ELLIPSIS;
}

struct current_varg current_varg;

void clear_current_varg()
{
  current_varg.m_number.m_gpr = 0;
  current_varg.m_number.m_fpr = 0;
  current_varg.m_offset.m_gpr = 0;
  current_varg.m_offset.m_fpr = 0;
  delete current_varg.m_counter.m_gpr;
  current_varg.m_counter.m_gpr = 0;
  delete current_varg.m_counter.m_fpr;
  current_varg.m_counter.m_fpr = 0;
  delete current_varg.m_pointer.m_gpr;
  current_varg.m_pointer.m_gpr = 0;
  delete current_varg.m_pointer.m_fpr;
  current_varg.m_pointer.m_fpr = 0;
}

int use_gpr(int n, const COMPILER::usr* entry)
{
  using namespace COMPILER;
  const type* T = entry->m_type;
  if ( T->real() )
    return n;
  T = T->promotion();
  int size = T->size();
  return n + (size < 8 ? 1 : 2);
}

bool use_fpr(const COMPILER::usr* entry)
{
        using namespace COMPILER;
        const type* T = entry->m_type;
        return T->real();
}

int decide_if_varg(int n, const COMPILER::type* T, const std::vector<COMPILER::usr*>& param)
{
  using namespace std;
  using namespace COMPILER;

  if ( !take_varg(T) )
    return n;

  int gpr = 8;
  gpr -= accumulate(param.begin(),param.end(),0,use_gpr);
  if ( gpr < 0 )
    gpr = 0;
  current_varg.m_number.m_gpr = gpr;

  int fpr = 8;
  fpr -= count_if(param.begin(),param.end(),use_fpr);
  if ( fpr < 0 )
    fpr = 0;
  current_varg.m_number.m_fpr = fpr;

  if ( !gpr && !fpr )
    return n;
  current_varg.m_offset.m_gpr = n;
  n += 4 * gpr;
  current_varg.m_offset.m_fpr = n;
  n += 8 * fpr;
  current_varg.m_counter.m_gpr = new ::stack(n,4); n += 4;
  current_varg.m_counter.m_fpr = new ::stack(n,4); n += 4;
  current_varg.m_pointer.m_gpr = new ::stack(n,4); n += 4;
  current_varg.m_pointer.m_fpr = new ::stack(n,4); n += 4;
  return n;
}

void save_if_varg()
{
  int gpr = current_varg.m_number.m_gpr;
  int fpr = current_varg.m_number.m_fpr;
  if ( !gpr && !fpr )
    return;

  if ( debug_flag )
    out << '\t' << "/* save varg */" << '\n';

  int offset = current_varg.m_offset.m_gpr;
  for ( int i = 8 - gpr + 3 ; i <= 10 ; ++i, offset += 4 )
    out << '\t' << "stw " << i << ',' << offset << "(31)" << '\n'; 

  std::string label = new_label();
  out << '\t' << "bne 1," << label << '\n';

  assert(offset == current_varg.m_offset.m_fpr);
  for ( int j = 8 - fpr + 1 ; j <= 8 ; ++j, offset += 8 )
    out << '\t' << "stfd " << j << ',' << offset << "(31)" << '\n'; 

  out << label << ":\n";
}
