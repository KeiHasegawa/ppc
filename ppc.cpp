#include "stdafx.h"

#ifdef CXX_GENERATOR
#include "cxx_core.h"
#else // CXX_GENERATOR
#include "c_core.h"
#endif // CXX_GENERATOR

#include "ppc.h"

extern "C" DLL_EXPORT int generator_seed()
{
#ifdef _MSC_VER
  int r = _MSC_VER;
#ifndef CXX_GENERATOR
  r += 10000000;
#else // CXX_GENERATOR
  r += 20000000;
#endif // CXX_GENERATOR
#ifdef WIN32
  r += 100000;
#endif // WIN32
#endif // _MSC_VER
#ifdef __GNUC__
  int r = (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__);
#ifndef CXX_GENERATOR
  r += 30000000;
#else // CXX_GENERATOR
  r += 40000000;
#endif // CXX_GENERATOR
#endif // __GNUC__
  return r;
}

extern "C" DLL_EXPORT
void generator_generate(COMPILER::generator::interface_t* ptr)
{
  genobj(ptr->m_root);
  if ( ptr->m_func )
    genfunc(ptr->m_func,*ptr->m_code);
  delete tmp_dword;
  tmp_dword = 0;
  delete tmp_word;
  tmp_word = 0;
}

class generator_option_table : public std::map<std::string, int (*)()> {
  static int set_debug_flag()
  {
    debug_flag = true;
    return 0;
  }
public:
  generator_option_table()
  {
    (*this)["--debug"] = set_debug_flag;
  }
} generator_option_table;

int option_handler(const char* option)
{
  std::map<std::string, int (*)()>::const_iterator p =
    generator_option_table.find(option);
  return p != generator_option_table.end() ? (p->second)() : 1;
}

extern "C" DLL_EXPORT void generator_option(int argc, const char** argv, int* error)
{
        using namespace std;
        ++argv;
        --argc;
#ifdef _MSC_VER
        const char** p; int* q;
        for (p = &argv[0], q = &error[0]; p != &argv[argc]; ++p, ++q)
                *q = option_handler(*p);
#else // _MSC_VER
        transform(&argv[0],&argv[argc],&error[0],option_handler);
#endif // _MSC_VER
}

std::ostream out(std::cout.rdbuf());

std::ofstream* ptr_out;

extern "C" DLL_EXPORT int generator_open_file(const char* fn)
{
  ptr_out = new std::ofstream(fn);
  out.rdbuf(ptr_out->rdbuf());
  return 0;
}

inline void destroy_address(std::pair<const COMPILER::var*, address*> pair)
{
    delete pair.second;
}

#ifdef CXX_GENERATOR
inline void call_simply(std::string func)
{
  out << '\t' << "bl " << func << '\n';
}
#endif // CXX_GENERATOR

extern "C" DLL_EXPORT int generator_close_file()
{
  using namespace std;
  if ( double_0x10000080000000.get() ){
    output_section(rom);
    out << '\t' << ".align" << '\t' << 8 << '\n';
    string label = double_0x10000080000000->label();
    out << label << ":\n";
    out << '\t' << ".long" << '\t' << "0x43300000\n";
    out << '\t' << ".long" << '\t' << "0x80000000\n";
  }
  if ( double_0x80000000.get() ){
    output_section(rom);
    out << '\t' << ".align" << '\t' << 8 << '\n';
    string label = double_0x80000000->label();
    out << label << ":\n";
    out << '\t' << ".long" << '\t' << "1105199104\n";
    out << '\t' << ".long" << '\t' << "0\n";
  }
  if ( double_0x100000000.get() ){
    output_section(rom);
    out << '\t' << ".align" << '\t' << 8 << '\n';
    string label = double_0x100000000->label();
    out << label << ":\n";
    out << '\t' << ".long" << '\t' << "1127219200\n";
    out << '\t' << ".long" << '\t' << "0\n";
  }

#ifdef CXX_GENERATOR
  if ( !ctors.empty() ){
    output_section(rom);
    out << '\t' << ".align" << '\t' << 4 << '\n';
    string label = new_label();
    out << label << ":\n";
    enter_helper(12);
    for_each(ctors.begin(),ctors.end(),call_simply);
    leave_helper();
    output_section(ctor);
    out << '\t' << ".long" << '\t' << label << '\n';
  }
  if ( !dtor_name.empty() ){
    output_section(rom);
    out << '\t' << ".align" << '\t' << 4 << '\n';
    string label = new_label();
    out << label << ":\n";
    enter_helper(12);
    call_simply(dtor_name);
    leave_helper();
    output_section(dtor);
    out << '\t' << ".long" << '\t' << label << '\n';
  }
#endif // CXX_GENERATOR
  delete ptr_out;
#ifdef _DEBUG
  for_each(address_descriptor.begin(), address_descriptor.end(),destroy_address);
#endif // _DEBUG
  return 0;
}

void(*output3ac)(std::ostream&, const COMPILER::tac*);

extern "C" DLL_EXPORT void generator_spell(void* arg)
{
        using namespace std;
        using namespace COMPILER;
        void* magic[] = {
                ((char **)arg)[0],
        };
        int index = 0;
        memcpy(&output3ac, &magic[index++], sizeof magic[0]);
}

bool string_literal(COMPILER::usr* entry)
{
  int c = entry->m_name[0];
  if ( c == 'L' )
    c = entry->m_name[1];
  return c == '"';
}

std::map<const COMPILER::var*, address*> address_descriptor;

void output_section(section kind)
{
  static section current;
  if ( current != kind ){
    current = kind;
    switch ( kind ){
    case rom: out << '\t' << ".text" << '\n'; break;
    case ram: out << '\t' << ".data" << '\n'; break;
    case ctor:
    case dtor:
      out << '\t' << ".section" << '\t';
      out << (kind == ctor ? ".ctors" : ".dtors");
      out << ',' << '"' << "aw" << '"' << '\n';
      break;
    }
  }
}

std::string new_label()
{
  static int cnt;
  std::ostringstream os;
  os << ".L" << ++cnt;
  return os.str();
}

std::map<COMPILER::var*, stack*> record_param;

char get_suffix(int size)
{
  switch ( size ){
  case 1: return 'b';
  case 2: return 'h';
  case 4: return 'w';
  }

  struct bad_size {};
  throw bad_size();
}

void mem::load(const reg& r, mode m) const
{
  r.m_kind == reg::fpr ? load_fpr(r) : load_gpr(r,m);
}

void mem::load_gpr(const reg& r, mode m) const
{
  int offset = 0;
  int size = m_size;
  if ( m == high ){
    assert(size == 8);
    size = 4;
  }
  else if ( size == 8 ){
    size = 4;
    offset += 4;
  }
  assert(r.m_no != 0);
  get(r);
  char suffix = get_suffix(size);
  out << '\t' << 'l' << suffix << 'z' << ' ' << r.m_no << ',';
  out << offset << '(' << r.m_no << ')' << '\n';
}

void mem::load_fpr(const reg& r) const
{
  reg tmp(reg::gpr,11);
  get(tmp);
  char suffix = m_size == 4 ? 's' : 'd';
  out << '\t' << "lf" << suffix << ' ' << r.m_no << ',';
  out << 0 << '(' << tmp.m_no << ')' << '\n';
}

void mem::store(const reg& r, mode m) const
{
  r.m_kind == reg::fpr ? store_fpr(r) : store_gpr(r,m);
}

void mem::store_gpr(const reg& r, mode m) const
{
  int offset = 0;
  int size = m_size;
  if ( m == high ){
    assert(size == 8);
    size = 4;
  }
  else if ( size == 8 ){
    size = 4;
    offset += 4;
  }
  assert(r.m_no != 0);
  reg t(reg::gpr,r.m_no == 9 ? 10 : 9);
  get(t);
  char suffix = get_suffix(size);
  out << '\t' << "st" << suffix << ' ' << r.m_no << ',';
  out << offset << '(' << t.m_no << ')' << '\n';
}

void mem::store_fpr(const reg& r) const
{
  reg tmp(reg::gpr,11);
  get(tmp);
  char suffix = m_size == 4 ? 's' : 'd';
  out << '\t' << "stf" << suffix << ' ' << r.m_no << ',';
  out << 0 << '(' << tmp.m_no << ')' << '\n';
}

void mem::get(const reg& r) const
{
  out << '\t' << "lis " << r.m_no << ',' << m_label << "@ha" << '\n';
  out << '\t' << "la " << r.m_no << ',' << m_label;
  out << "@l(" << r.m_no << ')' << '\n';
}

std::auto_ptr<mem> double_0x10000080000000;
std::auto_ptr<mem> double_0x80000000;
std::auto_ptr<mem> double_0x100000000;

bool character_constant(COMPILER::usr* entry)
{
  int c = entry->m_name[0];
  if ( c == '\'' )
    return true;

  if ( c == 'L' ){
    c = entry->m_name[1];
    if ( c == '\'' )
      return true;
  }
  return false;
}

bool integer_suffix(int c)
{
  return c == 'U' || c == 'L' || c == 'u' || c == 'l';
}

imm::imm(COMPILER::usr* entry) : m_pair(std::make_pair(0,0))
{
  using namespace std;
  if ( entry->m_name == "'\\0'" )
    m_value = "0";
  else if ( entry->m_name == "'\\a'" ){
    ostringstream os;
    os << int('\a');
    m_value = os.str();
  }
  else if ( entry->m_name == "'\\b'" ){
    ostringstream os;
    os << int('\b');
    m_value = os.str();
  }
  else if ( entry->m_name == "'\\t'" ){
    ostringstream os;
    os << int('\t');
    m_value = os.str();
  }
  else if ( entry->m_name == "'\\n'" ){
    ostringstream os;
    os << int('\n');
    m_value = os.str();
  }
  else if ( entry->m_name == "'\\v'" ){
    ostringstream os;
    os << int('\v');
    m_value = os.str();
  }
  else if ( entry->m_name == "'\\r'" ){
    ostringstream os;
    os << int('\r');
    m_value = os.str();
  }
  else if ( character_constant(entry) )
    m_value = entry->m_name;
#ifdef CXX_GENERATOR
  else if ( entry->m_name == "true" )
    m_value = "1";
  else if ( entry->m_name == "false" )
    m_value = "0";
#endif // CXX_GENERATOR
  else {
    m_value = entry->m_name;
    while ( integer_suffix(m_value[m_value.size()-1]) )
      m_value.erase(m_value.size()-1);
    int sign;
    if ( m_value[0] == '-' ){
      sign = -1;
      m_value.erase(0,1);
    }
    else
      sign = 1;
    istringstream is(m_value.c_str());
    unsigned int n;
    if ( m_value[0] == '0' && m_value.size() > 1 ){
      if ( m_value[1] == 'x' || m_value[1] == 'X' )
        is >> hex >> n;
      else
        is >> oct >> n;
    }
    else
      is >> n;
    n *= sign;
    int m = int(n) << 16 >> 16;
    if ( n != m ){
      m_value.erase();
      m_pair.first = n >> 16;
      m_pair.second = n & 0xffff;
    }
    else if ( sign == -1 )
      m_value = string("-") + m_value;
  }
}

void imm::load(const reg& r, mode m) const
{
  assert(m != high);
  if ( !m_value.empty() )
    out << '\t' << "li " << r.m_no << ',' << m_value << '\n';
  else {
    out << '\t' << "lis " << r.m_no << ',' << m_pair.first << '\n';
    out << '\t' << "ori " << r.m_no << ',' << r.m_no << ',';
    out << m_pair.second << '\n';
  }
}

void stack::load(const reg& r, mode m) const
{
  r.m_kind == reg::fpr ? load_fpr(r) : load_gpr(r,m);
}

void stack::load_gpr(const reg& r, mode m) const
{
  int offset = m_offset;
  int size = m_size;
  if ( m == high ){
    assert(size == 8);
    size = 4;
  }
  else if ( size == 8 ){
    size = 4;
    offset += 4;
  }
  char suffix = get_suffix(size);
  out << '\t' << "l" << suffix << 'z' << ' ' << r.m_no << ',';
  out << offset << "(31)" << '\n';
}

void stack::load_fpr(const reg& r) const
{
  char suffix = m_size == 4 ? 's' : 'd';
  out << '\t' << "lf" << suffix << ' ' << r.m_no << ',';
  out << m_offset << "(31)" << '\n';
}

void stack::store(const reg& r, mode m) const
{
  r.m_kind == reg::fpr ? store_fpr(r) : store_gpr(r,m);
}

void stack::store_gpr(const reg& r, mode m) const
{
  int offset = m_offset;
  int size = m_size;
  if ( m == high ){
    assert(size == 8);
    size = 4;
  }
  else if ( size == 8 ){
    size = 4;
    offset += 4;
  }
  char suffix = get_suffix(size);
  out << '\t' << "st" << suffix << ' ' << r.m_no << ',';
  out << offset << "(31)" << '\n';
}

void stack::store_fpr(const reg& r) const
{
  char suffix = m_size == 4 ? 's' : 'd';
  out << '\t' << "stf" << suffix << ' ' << r.m_no << ',';
  out << m_offset << "(31)" << '\n';
}

void stack::get(const reg& r) const
{
  if ( m_size < 0 )
    out << '\t' << "lwz " << r.m_no << ',' << m_offset << "(31)\n";
  else {
    out << '\t' << "mr " << r.m_no << ',' << 31 << '\n';
    assert(r.m_no != 0);
    out << '\t' << "addi " << r.m_no << ',' << r.m_no << ',';
    out << m_offset << '\n';
  }
}

stack* tmp_dword;
stack* tmp_word;

#ifdef CXX_GENERATOR
std::string signature(const COMPILER::type* T)
{
  using namespace std;
  using namespace COMPILER;
  ostringstream os;
  assert(T->m_id == type::FUNC);
  typedef const func_type FT;
  FT* ft = static_cast<FT*>(T);
  const vector<const type*>& param = ft->param();
  for (auto T : param)
    T->encode(os);
  return os.str();
}
#endif // CXX_GENERATOR

struct function_exit function_exit;

bool debug_flag;
