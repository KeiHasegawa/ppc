#include "stdafx.h"

#include "c_core.h"

#define COMPILER c_compiler

#include "ppc.h"
#include "gencode.h"

struct gencode::table : std::map<COMPILER::tac::id_t, void (*)(const COMPILER::tac*)> {
  table()
  {
    using namespace COMPILER;
    (*this)[tac::ASSIGN] = assign;
    (*this)[tac::ADD] = add;
    (*this)[tac::SUB] = sub;
    (*this)[tac::MUL] = mul;
    (*this)[tac::DIV] = div;
    (*this)[tac::MOD] = mod;
    (*this)[tac::LSH] = lsh;
    (*this)[tac::RSH] = rsh;
    (*this)[tac::AND] = _and;
    (*this)[tac::XOR] = _xor;
    (*this)[tac::OR] = _or;
    (*this)[tac::UMINUS] = uminus;
    (*this)[tac::TILDE] = tilde;
    (*this)[tac::CAST] = tc;
    (*this)[tac::ADDR] = addr;
    (*this)[tac::INVLADDR] = invladdr;
    (*this)[tac::INVRADDR] = invraddr;
    (*this)[tac::LOFF] = loff;
    (*this)[tac::ROFF] = roff;
    (*this)[tac::PARAM] = param;
    (*this)[tac::CALL] = call;
    (*this)[tac::RETURN] = _return;
    (*this)[tac::GOTO] = _goto;
    (*this)[tac::TO] = to;
    (*this)[tac::ALLOC] = alloc;
    (*this)[tac::ALLOC] = dealloc;
    (*this)[tac::ASM] = asm_;
    (*this)[tac::VASTART] = _va_start;
    (*this)[tac::VAARG] = _va_arg;
    (*this)[tac::VAEND] = _va_end;
  }
};

gencode::table gencode::m_table;

inline bool cmp_id(const COMPILER::tac* ptr, COMPILER::tac::id_t id)
{
  return ptr->m_id == id;
}

address* getaddr(COMPILER::var* entry)
{
  using namespace std;
  using namespace COMPILER;
  map<const var*, address*>::const_iterator p = address_descriptor.find(entry);
  assert(p != address_descriptor.end());
  return p->second;
}

void copy(address* dst, address* src, int size)
{
  if (dst && src) {
    reg r3(reg::gpr, 3);
    dst->get(r3);
    reg r4(reg::gpr, 4);
    src->get(r4);
  }
  else if (dst) {
    reg r3(reg::gpr, 3);
    dst->get(r3);
  }
  else if (src) {
    reg r4(reg::gpr, 4);
    src->get(r4);
  }
  out << '\t' << "li 5," << size << '\n';
  out << '\t' << "bl " << "memcpy" << '\n';
}

void copy_record_param(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  var* entry = tac->y;
  const type* T = entry->m_type;
  if ( T->scalar() )
    return;

  stack* dst = record_param[entry];
  address* src = getaddr(entry);
  int size = T->size();
  copy(dst, src, size);
}

int gencode::operator()(int n, const COMPILER::tac* ptr)
{
  using namespace std;
  using namespace COMPILER;

  if ( debug_flag ){
    out << '\t' << "/* ";
    output3ac(out, ptr);
    out << " */" << '\n';
  }

  if ( cmp_id(ptr, tac::PARAM) && !m_record_stuff ){
    vector<tac*>::const_iterator p =
      find_if(m_v3ac.begin() + n, m_v3ac.end() ,bind2nd(ptr_fun(cmp_id),tac::CALL));
    for_each(m_v3ac.begin() + n, p, copy_record_param);
    prepare_aggregate_return(*p);
    m_record_stuff = true;
  }
  else if ( cmp_id(ptr, tac::CALL) )
    m_record_stuff = false;

  m_table[ptr->m_id](ptr);
  return n + 1;
}

void gencode::assign(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  T->scalar() ? assign_scalar(tac) : assign_aggregate(tac);
}

void gencode::assign_scalar(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  const type* T = tac->x->m_type;
  int size = T->size();
  reg::kind kind = T->real() || size == 8 ? reg::fpr : reg::gpr;
  reg r(kind,9);
  y->load(r);
  x->store(r);
}

void gencode::assign_aggregate(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  const type* T = tac->x->m_type;
  int size = T->size();
  copy(x, y, size);
}

void gencode::add(const COMPILER::tac* tac)
{
  using namespace COMPILER;  
  const type* T = tac->x->m_type;
  if ( T->real() )
    binop_real(tac,"fadd");
  else if ( T->size() > 4 )
    binop_longlong(tac,"addc","adde");
  else
    binop_notlonglong(tac,"add");
}

void gencode::sub(const COMPILER::tac* tac)
{
  using namespace COMPILER;  
  const type* T = tac->x->m_type;
  if ( T->real() )
    binop_real(tac,"fsub");
  else if ( T->size() > 4 )
    binop_longlong(tac,"subc","sube");
  else
    binop_notlonglong(tac,"sub");
}

void gencode::mul(const COMPILER::tac* tac)
{
  using namespace COMPILER;  
  const type* T = tac->x->m_type;
  if ( T->real() )
    binop_real(tac,"fmul");
  else if ( T->size() > 4 )
    mul_longlong(tac);
  else
    binop_notlonglong(tac,"mullw");
}

void gencode::mul_longlong(const COMPILER::tac* tac)
{
  using namespace COMPILER;  
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  
  reg s(reg::gpr,9);
  reg t(reg::gpr,0);
  reg u(reg::gpr,11);
  reg v(reg::gpr,12);

  y->load(s,address::low);
  z->load(t,address::low);
  out << '\t' << "mulhwu " << u.m_no << ',' << s.m_no << ',' << t.m_no << '\n';
  out << '\t' << "mullw "  << v.m_no << ',' << s.m_no << ',' << t.m_no << '\n';
  y->load(s,address::low);
  z->load(t,address::high);
  out << '\t' << "mullw "  << s.m_no << ',' << s.m_no << ',' << t.m_no << '\n';
  out << '\t' << "mr "     << t.m_no << ',' << u.m_no << '\n';
  out << '\t' << "add "    << t.m_no << ',' << t.m_no << ',' << s.m_no << '\n';
  y->load(s,address::high);
  z->load(u,address::low);
  out << '\t' << "mullw "  << s.m_no << ',' << u.m_no << ',' << s.m_no << '\n';
  out << '\t' << "add "    << t.m_no << ',' << t.m_no << ',' << s.m_no << '\n';

  x->store(v,address::low);
  x->store(t,address::high);
}

void gencode::div(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->real() )
    binop_real(tac,"fdiv");
  else if ( T->size() > 4 ){
    string func = !T->_signed() ? "__udivdi3" : "__divdi3";
    runtime_longlong(tac,func);
  }
  else {
    string op = !T->_signed() ? "divwu" : "divw";
    binop_notlonglong(tac,op);
  }
}

void gencode::mod(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->size() > 4 ){
    std::string func = !T->_signed() ? "__umoddi3" : "__moddi3";
    return runtime_longlong(tac,func);
  }

  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);

  reg ry(reg::gpr,11);
  reg rz(reg::gpr,9);
  reg tmp(reg::gpr,0);
  y->load(ry);
  z->load(rz);
  string op = !T->_signed() ? "divwu" : "divw";
  out << '\t' << op << ' ' << tmp.m_no << ',' << ry.m_no << ',' << rz.m_no << '\n';
  z->load(rz);
  out << '\t' << "mullw "  << rz.m_no << ',' << tmp.m_no << ',' << rz.m_no << '\n';
  out << '\t' << "sub "    << rz.m_no << ',' << ry.m_no  << ',' << rz.m_no << '\n';
  x->store(rz);
}

void gencode::lsh(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->size() > 4 )
    runtime_longlong_shift(tac,"__ashldi3");
  else
    binop_notlonglong(tac,"slw");
}

void gencode::rsh(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;  
  const type* T = tac->x->m_type;
  if ( T->size() > 4 ){
    string func = !T->_signed() ? "__lshrdi3" : "__ashrdi3";
    runtime_longlong_shift(tac,func);
  }
  else {
    string op = !T->_signed() ? "srw" : "sraw";
    binop_notlonglong(tac,op);
  }
}

void gencode::runtime_longlong_shift(const COMPILER::tac* tac, std::string func)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);

  reg r3(reg::gpr,3);
  reg r4(reg::gpr,4);
  reg r5(reg::gpr,5);
  reg r6(reg::gpr,6);
  y->load(r3,address::high);
  y->load(r4,address::low);
  z->load(r5,address::high);
  z->load(r6,address::low);
  const type* T = tac->z->m_type;
  int size = T->size();
  if ( size > 4 )
    out << '\t' << "mr " << r5.m_no << ',' << r6.m_no << '\n';
  out << '\t' << "bl " << func << '\n';
  x->store(r3,address::high);
  x->store(r4,address::low);
}

void gencode::_and(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->size() > 4 )
    binop_longlong(tac,"and","and");
  else
    binop_notlonglong(tac,"and");
}

void gencode::_xor(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->size() > 4 )
    binop_longlong(tac,"xor","xor");
  else
    binop_notlonglong(tac,"xor");
}

void gencode::_or(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->size() > 4 )
    binop_longlong(tac,"or","or");
  else
    binop_notlonglong(tac,"or");
}

void gencode::binop_notlonglong(const COMPILER::tac* tac, std::string op)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  reg yy(reg::gpr,9);
  y->load(yy);
  reg zz(reg::gpr,10);
  z->load(zz);
  out << '\t' << op << ' ' << zz.m_no << ',' << yy.m_no << ',' << zz.m_no << '\n';
  x->store(zz);
}

void gencode::binop_real(const COMPILER::tac* tac, std::string op)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  reg yy(reg::fpr,1);
  y->load(yy);
  reg zz(reg::fpr,2);
  z->load(zz);
  const type* T = tac->x->m_type;
  std::string suffix;
  if ( T->size() == 4 )
    suffix = "s";
  out << '\t' << op << suffix << ' ';
  out << zz.m_no << ',' << yy.m_no << ',' << zz.m_no << '\n';
  x->store(zz);
}

void gencode::binop_longlong(const COMPILER::tac* tac, std::string lo, std::string hi)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);

  reg a(reg::gpr, 9); y->load(a,address::low);
  reg b(reg::gpr,10); y->load(b,address::high);
  reg c(reg::gpr,11); z->load(c,address::low);
  reg d(reg::gpr,12); z->load(d,address::high);
  out << '\t' << lo << ' ' << a.m_no << ',' << a.m_no << ',' << c.m_no << '\n';
  if ( hi != "sube" )
    out << '\t' << hi << ' ' << b.m_no << ',' << b.m_no << ',' << d.m_no;
  else
    out << '\t' << "subfe " << ' ' << b.m_no << ',' << d.m_no << ',' << b.m_no;
  out << '\n';    
  x->store(a,address::low);
  x->store(b,address::high);
}

void gencode::runtime_longlong(const COMPILER::tac* tac, std::string func)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);

  reg r3(reg::gpr,3);
  reg r4(reg::gpr,4);
  reg r5(reg::gpr,5);
  reg r6(reg::gpr,6);
  
  y->load(r3,address::high);
  y->load(r4,address::low);
  z->load(r5,address::high);
  z->load(r6,address::low);
  
  out << '\t' << "bl " << func << '\n';
  
  x->store(r3,address::high);
  x->store(r4,address::low);
}

void gencode::uminus(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  if ( T->real() )
    uminus_real(tac);
  else if ( T->size() > 4 )
    uminus_longlong(tac);
  else
    uminus_notlonglong(tac);
}

void gencode::uminus_notlonglong(const COMPILER::tac* tac)
{
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  reg r(reg::gpr,11);
  y->load(r);
  out << '\t' << "neg " << r.m_no << ',' << r.m_no << '\n';
  x->store(r);
}

void gencode::uminus_real(const COMPILER::tac* tac)
{
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  reg r(reg::fpr,0);
  y->load(r);
  out << '\t' << "fneg " << r.m_no << ',' << r.m_no << '\n';
  x->store(r);
}

void gencode::uminus_longlong(const COMPILER::tac* tac)
{
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);

  reg hi(reg::gpr, 9); y->load(hi,address::high);
  reg lo(reg::gpr,10); y->load(lo,address::low);

  out << '\t' << "subfic " << lo.m_no << ',' << lo.m_no << ',' << "0\n";
  out << '\t' << "subfze " << hi.m_no << ',' << hi.m_no << '\n';

  x->store(hi,address::high);
  x->store(lo,address::low);
}

void gencode::tilde(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);

  reg r(reg::gpr,9);
  y->load(r);
  out << '\t' << "nor " << r.m_no << ',' << r.m_no << ',' << r.m_no << '\n';
  x->store(r);

  const type* T = tac->x->m_type;
  int size = T->size();
  if ( size <= 4 )
    return;
  y->load(r,address::high);
  out << '\t' << "nor " << r.m_no << ',' << r.m_no << ',' << r.m_no << '\n';
  x->store(r,address::high);
}

struct gencode::tc_table : std::map<std::pair<int,int>, void (*)(const COMPILER::tac*)> {
public:
  tc_table()
  {
        using namespace std;
    (*this)[make_pair(4,4)] = sint08_sint08;
    (*this)[make_pair(4,6)] = sint08_uint08;
    (*this)[make_pair(4,8)] = sint08_sint16;
    (*this)[make_pair(4,10)] = sint08_uint16;
    (*this)[make_pair(4,16)] = sint08_sint32;
    (*this)[make_pair(4,18)] = sint08_uint32;
    (*this)[make_pair(4,32)] = sint08_sint64;
    (*this)[make_pair(4,34)] = sint08_uint64;
    (*this)[make_pair(4,17)] = sint08_single;
    (*this)[make_pair(4,33)] = sint08_double;

    (*this)[make_pair(6,4)] = uint08_sint08;
    (*this)[make_pair(6,8)] = uint08_sint16;
    (*this)[make_pair(6,10)] = uint08_uint16;
    (*this)[make_pair(6,16)] = uint08_sint32;
    (*this)[make_pair(6,18)] = uint08_uint32;
    (*this)[make_pair(6,32)] = uint08_sint64;
    (*this)[make_pair(6,34)] = uint08_uint64;
    (*this)[make_pair(6,17)] = uint08_single;
    (*this)[make_pair(6,33)] = uint08_double;

    (*this)[make_pair(8,4)] = sint16_sint08;
    (*this)[make_pair(8,6)] = sint16_uint08;
    (*this)[make_pair(8,10)] = sint16_uint16;
    (*this)[make_pair(8,16)] = sint16_sint32;
    (*this)[make_pair(8,18)] = sint16_uint32;
    (*this)[make_pair(8,32)] = sint16_sint64;
    (*this)[make_pair(8,34)] = sint16_uint64;
    (*this)[make_pair(8,17)] = sint16_single;
    (*this)[make_pair(8,33)] = sint16_double;

    (*this)[make_pair(10,4)] = uint16_sint08;
    (*this)[make_pair(10,6)] = uint16_uint08;
    (*this)[make_pair(10,8)] = uint16_sint16;
    (*this)[make_pair(10,16)] = uint16_sint32;
    (*this)[make_pair(10,18)] = uint16_uint32;
    (*this)[make_pair(10,32)] = uint16_sint64;
    (*this)[make_pair(10,34)] = uint16_uint64;
    (*this)[make_pair(10,17)] = uint16_single;
    (*this)[make_pair(10,33)] = uint16_double;

    (*this)[make_pair(16,4)] = sint32_sint08;
    (*this)[make_pair(16,6)] = sint32_uint08;
    (*this)[make_pair(16,8)] = sint32_sint16;
    (*this)[make_pair(16,10)] = sint32_uint16;
    (*this)[make_pair(16,16)] = sint32_sint32;
    (*this)[make_pair(16,18)] = sint32_uint32;
    (*this)[make_pair(16,32)] = sint32_sint64;
    (*this)[make_pair(16,34)] = sint32_uint64;
    (*this)[make_pair(16,17)] = sint32_single;
    (*this)[make_pair(16,33)] = sint32_double;

    (*this)[make_pair(18,4)] = uint32_sint08;
    (*this)[make_pair(18,6)] = uint32_uint08;
    (*this)[make_pair(18,8)] = uint32_sint16;
    (*this)[make_pair(18,10)] = uint32_uint16;
    (*this)[make_pair(18,16)] = uint32_sint32;
    (*this)[make_pair(18,18)] = uint32_uint32;
    (*this)[make_pair(18,32)] = uint32_sint64;
    (*this)[make_pair(18,34)] = uint32_uint64;
    (*this)[make_pair(18,17)] = uint32_single;
    (*this)[make_pair(18,33)] = uint32_double;

    (*this)[make_pair(32,4)] = sint64_sint08;
    (*this)[make_pair(32,6)] = sint64_uint08;
    (*this)[make_pair(32,8)] = sint64_sint16;
    (*this)[make_pair(32,10)] = sint64_uint16;
    (*this)[make_pair(32,16)] = sint64_sint32;
    (*this)[make_pair(32,18)] = sint64_uint32;
    (*this)[make_pair(32,34)] = sint64_uint64;
    (*this)[make_pair(32,17)] = sint64_single;
    (*this)[make_pair(32,33)] = sint64_double;

    (*this)[make_pair(34,4)] = uint64_sint08;
    (*this)[make_pair(34,6)] = uint64_uint08;
    (*this)[make_pair(34,8)] = uint64_sint16;
    (*this)[make_pair(34,10)] = uint64_uint16;
    (*this)[make_pair(34,16)] = uint64_sint32;
    (*this)[make_pair(34,18)] = uint64_uint32;
    (*this)[make_pair(34,32)] = uint64_sint64;
    (*this)[make_pair(34,17)] = uint64_single;
    (*this)[make_pair(34,33)] = uint64_double;
    
    (*this)[make_pair(17,4)] = single_sint08;
    (*this)[make_pair(17,6)] = single_uint08;
    (*this)[make_pair(17,8)] = single_sint16;
    (*this)[make_pair(17,10)] = single_uint16;
    (*this)[make_pair(17,16)] = single_sint32;
    (*this)[make_pair(17,18)] = single_uint32;
    (*this)[make_pair(17,32)] = single_sint64;
    (*this)[make_pair(17,34)] = single_uint64;
    (*this)[make_pair(17,33)] = single_double;

    (*this)[make_pair(33,4)] = double_sint08;
    (*this)[make_pair(33,6)] = double_uint08;
    (*this)[make_pair(33,8)] = double_sint16;
    (*this)[make_pair(33,10)] = double_uint16;
    (*this)[make_pair(33,16)] = double_sint32;
    (*this)[make_pair(33,18)] = double_uint32;
    (*this)[make_pair(33,32)] = double_sint64;
    (*this)[make_pair(33,34)] = double_uint64;
    (*this)[make_pair(33,17)] = double_single;
    (*this)[make_pair(33,33)] = double_double;
  }
};

gencode::tc_table gencode::m_tc_table;

void gencode::tc(const COMPILER::tac* tac)
{
  using namespace std;
  int x = tc_id(tac->x->m_type);
  int y = tc_id(tac->y->m_type);
  m_tc_table[make_pair(x,y)](tac);
}

int gencode::tc_id(const COMPILER::type* T)
{
  int n = (T->integer() && !T->_signed()) ? 1 : 0;
  return (T->size() << 2) + (n << 1) + T->real();
}

void gencode::sint08_sint08(const COMPILER::tac* tac)
{
  assign(tac);
}

void gencode::sint08_uint08(const COMPILER::tac* tac)
{
  assign(tac);
}

void gencode::sint08_sint16(const COMPILER::tac* tac)
{
  extend(tac,true,8);
}

void gencode::sint08_uint16(const COMPILER::tac* tac)
{
  extend(tac,true,8);
}

void gencode::sint08_sint32(const COMPILER::tac* tac)
{
  extend(tac,true,8);
}

void gencode::sint08_uint32(const COMPILER::tac* tac)
{
  extend(tac,true,8);
}

void gencode::sint08_sint64(const COMPILER::tac* tac)
{
  extend(tac,true,8);
}

void gencode::sint08_uint64(const COMPILER::tac* tac)
{
  extend(tac,true,8);
}

void gencode::sint08_single(const COMPILER::tac* tac)
{
  common_real(tac,true,8);
}

void gencode::sint08_double(const COMPILER::tac* tac)
{
  common_real(tac,true,8);
}

void gencode::uint08_sint08(const COMPILER::tac* tac)
{
  assign(tac);
}

void gencode::uint08_sint16(const COMPILER::tac* tac)
{
  extend(tac,false,8);
}

void gencode::uint08_uint16(const COMPILER::tac* tac)
{
  extend(tac,false,8);
}

void gencode::uint08_sint32(const COMPILER::tac* tac)
{
  extend(tac,false,8);
}

void gencode::uint08_uint32(const COMPILER::tac* tac)
{
  extend(tac,false,8);
}

void gencode::uint08_sint64(const COMPILER::tac* tac)
{
  extend(tac,false,8);
}

void gencode::uint08_uint64(const COMPILER::tac* tac)
{
  extend(tac,false,8);
}

void gencode::uint08_single(const COMPILER::tac* tac)
{
  common_real(tac,false,8);
}

void gencode::uint08_double(const COMPILER::tac* tac)
{
  common_real(tac,false,8);
}

void gencode::sint16_sint08(const COMPILER::tac* tac)
{
  extend(tac,true,8);
}

void gencode::sint16_uint08(const COMPILER::tac* tac)
{
  extend(tac,false,8);
}

void gencode::sint16_uint16(const COMPILER::tac* tac)
{
  assign(tac);
}

void gencode::sint16_sint32(const COMPILER::tac* tac)
{
  extend(tac,true,16);
}

void gencode::sint16_uint32(const COMPILER::tac* tac)
{
  extend(tac,true,16);
}

void gencode::sint16_sint64(const COMPILER::tac* tac)
{
  extend(tac,true,16);
}

void gencode::sint16_uint64(const COMPILER::tac* tac)
{
  extend(tac,true,16);
}

void gencode::sint16_single(const COMPILER::tac* tac)
{
  common_real(tac,true,16);
}

void gencode::sint16_double(const COMPILER::tac* tac)
{
  common_real(tac,true,16);
}

void gencode::uint16_sint08(const COMPILER::tac* tac)
{
  extend(tac,true,8);
}

void gencode::uint16_uint08(const COMPILER::tac* tac)
{
  extend(tac,false,8);
}

void gencode::uint16_sint16(const COMPILER::tac* tac)
{
  assign(tac);
}

void gencode::uint16_sint32(const COMPILER::tac* tac)
{
  extend(tac,false,16);
}

void gencode::uint16_uint32(const COMPILER::tac* tac)
{
  extend(tac,false,16);
}

void gencode::uint16_sint64(const COMPILER::tac* tac)
{
  extend(tac,false,16);
}

void gencode::uint16_uint64(const COMPILER::tac* tac)
{
  extend(tac,false,16);
}

void gencode::uint16_single(const COMPILER::tac* tac)
{
  common_real(tac,false,16);
}

void gencode::uint16_double(const COMPILER::tac* tac)
{
  common_real(tac,false,16);
}

void gencode::sint32_sint08(const COMPILER::tac* tac)
{
  extend(tac,true,8);
}

void gencode::sint32_uint08(const COMPILER::tac* tac)
{
  extend(tac,false,8);
}

void gencode::sint32_sint16(const COMPILER::tac* tac)
{
  extend(tac,true,16);
}

void gencode::sint32_uint16(const COMPILER::tac* tac)
{
  extend(tac,false,16);
}

void gencode::sint32_sint32(const COMPILER::tac* tac)
{
  assign(tac);
}

void gencode::sint32_uint32(const COMPILER::tac* tac)
{
  assign(tac);
}

void gencode::sint32_sint64(const COMPILER::tac* tac)
{
  common_longlong(tac);
}

void gencode::sint32_uint64(const COMPILER::tac* tac)
{
  common_longlong(tac);
}

void gencode::sint32_single(const COMPILER::tac* tac)
{
  common_real(tac,true,32);
}

void gencode::sint32_double(const COMPILER::tac* tac)
{
  common_real(tac,true,32);
}

void gencode::uint32_sint08(const COMPILER::tac* tac)
{
  extend(tac,true,8);
}

void gencode::uint32_uint08(const COMPILER::tac* tac)
{
  extend(tac,false,8);
}

void gencode::uint32_sint16(const COMPILER::tac* tac)
{
  extend(tac,true,16);
}

void gencode::uint32_uint16(const COMPILER::tac* tac)
{
  extend(tac,false,16);
}

void gencode::uint32_sint32(const COMPILER::tac* tac)
{
  assign(tac);
}

void gencode::uint32_uint32(const COMPILER::tac* tac)
{
  assign(tac);
}

void gencode::uint32_sint64(const COMPILER::tac* tac)
{
  common_longlong(tac);
}

void gencode::uint32_uint64(const COMPILER::tac* tac)
{
  common_longlong(tac);
}

void gencode::uint32_single(const COMPILER::tac* tac)
{
  uint32_real(tac);
}

void gencode::uint32_double(const COMPILER::tac* tac)
{
  uint32_real(tac);
}

void gencode::uint32_real(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;

  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);

  reg a(reg::fpr,13);
  y->load(a);
  reg b(reg::fpr,0);
  if ( !double_0x80000000.get() ){
    const type* T = tac->y->m_type->varg();
        int size = T->size();
    double_0x80000000 = auto_ptr<mem>(new mem(new_label(),size));
  }
  double_0x80000000->load(b);
  out << '\t' << "fcmpu 7," << a.m_no << ',' << b.m_no << '\n';
  out << '\t' << "cror 30, 29, 30" << '\n';
  string label = new_label();
  out << '\t' << "beq 7, " << label << '\n';

  const reg& fpr = b;
  y->load(fpr);
  out << '\t' << "fctiwz " << fpr.m_no << ',' << fpr.m_no << '\n';
  tmp_dword->store(fpr);
  reg gpr(reg::gpr,9);
  tmp_dword->load(gpr);
  x->store(gpr);
  string label2 = new_label();
  out << '\t' << "b " << label2 << '\n';
  
  out << label << ":\n";
  y->load(a);
  double_0x80000000->load(b);
  out << '\t' << "fsub " << b.m_no << ',' << a.m_no << ',' << b.m_no << '\n';
  out << '\t' << "fctiwz " << b.m_no << ',' << b.m_no << '\n';
  tmp_dword->store(b);
  tmp_dword->load(gpr);
  out << '\t' << "xoris " << gpr.m_no << ',' << gpr.m_no << ',' << "0x8000\n";
  x->store(gpr);
  out << label2 << ":\n";
}

void gencode::sint64_sint08(const COMPILER::tac* tac)
{
  extend64(tac,true,8);
}

void gencode::sint64_uint08(const COMPILER::tac* tac)
{
  extend64(tac,false,8);
}

void gencode::sint64_sint16(const COMPILER::tac* tac)
{
  extend64(tac,true,16);
}

void gencode::sint64_uint16(const COMPILER::tac* tac)
{
  extend64(tac,false,16);
}

void gencode::sint64_sint32(const COMPILER::tac* tac)
{
  extend64(tac,true,32);
}

void gencode::sint64_uint32(const COMPILER::tac* tac)
{
  extend64(tac,false,32);
}

void gencode::sint64_uint64(const COMPILER::tac* tac)
{
  assign(tac);
}

void gencode::sint64_single(const COMPILER::tac* tac)
{
  longlong_real(tac,"__fixsfdi");
}

void gencode::sint64_double(const COMPILER::tac* tac)
{
  longlong_real(tac,"__fixdfdi");
}

void gencode::uint64_sint08(const COMPILER::tac* tac)
{
  extend64(tac,true,8);
}

void gencode::uint64_uint08(const COMPILER::tac* tac)
{
  extend64(tac,false,8);
}

void gencode::uint64_sint16(const COMPILER::tac* tac)
{
  extend64(tac,true,16);
}

void gencode::uint64_uint16(const COMPILER::tac* tac)
{
  extend64(tac,false,16);
}

void gencode::uint64_sint32(const COMPILER::tac* tac)
{
  extend64(tac,true,32);
}

void gencode::uint64_uint32(const COMPILER::tac* tac)
{
  extend64(tac,false,32);
}

void gencode::uint64_sint64(const COMPILER::tac* tac)
{
  assign(tac);
}

void gencode::uint64_single(const COMPILER::tac* tac)
{
  longlong_real(tac,"__fixunssfdi");
}

void gencode::uint64_double(const COMPILER::tac* tac)
{
  longlong_real(tac,"__fixunsdfdi");
}

void gencode::extend64(const COMPILER::tac* tac, bool signextend, int bit)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);

  reg r(reg::gpr,9);
  y->load(r,address::low);
  if ( bit != 32 ){
    if ( signextend ){
      char suffix = get_suffix(bit/8);
      out << '\t' << "exts" << suffix << ' ';
      out << r.m_no << ',' << r.m_no << '\n';
    }
    else {
      out << '\t' << "rlwinm " << r.m_no << ',' << r.m_no << ',';
      out << 0 << ',' << ((1 << bit)-1) << '\n';
    }
  }
  x->store(r,address::low);
  if ( signextend )
    out << '\t' << "srawi " << r.m_no << ',' << r.m_no << ',' << 31 << '\n';
  else
    out << '\t' << "li " << r.m_no << ',' << "0\n";
  x->store(r,address::high);
}

void gencode::single_sint08(const COMPILER::tac* tac)
{
  real_common(tac,true,8);
}

void gencode::single_uint08(const COMPILER::tac* tac)
{
  real_common(tac,false,8);
}

void gencode::single_sint16(const COMPILER::tac* tac)
{
  real_common(tac,true,16);
}

void gencode::single_uint16(const COMPILER::tac* tac)
{
  real_common(tac,false,16);
}

void gencode::single_sint32(const COMPILER::tac* tac)
{
  real_common(tac,true,32);
}

void gencode::single_uint32(const COMPILER::tac* tac)
{
  real_uint32(tac);
}

void gencode::single_sint64(const COMPILER::tac* tac)
{
  real_sint64(tac,"__floatdisf");
}

void gencode::single_uint64(const COMPILER::tac* tac)
{
  real_uint64(tac,"__floatdisf","fadds");
}

void gencode::single_double(const COMPILER::tac* tac)
{
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  reg r(reg::fpr,0);
  y->load(r);
  out << '\t' << "frsp " << r.m_no << ',' << r.m_no << '\n';
  x->store(r);
}

void gencode::double_sint08(const COMPILER::tac* tac)
{
  real_common(tac,true,8);
}

void gencode::double_uint08(const COMPILER::tac* tac)
{
  real_common(tac,false,8);
}

void gencode::double_sint16(const COMPILER::tac* tac)
{
  real_common(tac,true,16);
}

void gencode::double_uint16(const COMPILER::tac* tac)
{
  real_common(tac,false,16);
}

void gencode::double_sint32(const COMPILER::tac* tac)
{
  real_common(tac,true,32);
}

void gencode::real_common(const COMPILER::tac* tac, bool signextend, int bit)
{
  using namespace std;
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);

  if ( typeid(*y) == typeid(imm) ){
    reg r(reg::gpr,9);
    y->load(r);
    tmp_word->store(r);
    y = tmp_word;
  }
  else if ( bit != 32 ){
    reg r(reg::gpr,9);
    y->load(r);
    if ( signextend ){
      std::string op = bit == 8 ? "extsb" : "extsh";
      out << '\t' << op << ' ' << r.m_no << ',' << r.m_no << '\n';
    }
    tmp_word->store(r);
    y = tmp_word;
  }
  
  reg r(reg::gpr,9);
  y->load(r);
  out << '\t' << "xoris " << r.m_no << ',' << r.m_no << ',' << "0x8000\n";
  tmp_dword->store(r,address::low);

  out << '\t' << "lis " << r.m_no << ',' << "0x4330\n";
  tmp_dword->store(r,address::high);

  reg a(reg::fpr,0); tmp_dword->load(a);

  reg b(reg::fpr,13);
  if ( !double_0x10000080000000.get() ){
    const type* T = tac->x->m_type->varg();
        int size = T->size();
    double_0x10000080000000 = auto_ptr<mem>(new mem(new_label(),size));
  }
  double_0x10000080000000->load(b);

  out << '\t' << "fsub " << a.m_no << ',' << a.m_no << ',' << b.m_no << '\n';
  x->store(a);
}

void gencode::double_uint32(const COMPILER::tac* tac)
{
  real_uint32(tac);
}

void gencode::double_sint64(const COMPILER::tac* tac)
{
  real_sint64(tac,"__floatdidf");
}

void gencode::double_uint64(const COMPILER::tac* tac)
{
  real_uint64(tac,"__floatdidf","fadd");
}

void gencode::double_single(const COMPILER::tac* tac)
{
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  reg r(reg::fpr,0);
  y->load(r);
  x->store(r);
}

void gencode::double_double(const COMPILER::tac* tac)
{
  assign(tac);
}

void gencode::extend(const COMPILER::tac* tac, bool signextend, int bit)
{
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  reg r(reg::gpr,9);
  y->load(r);
  if ( signextend ){
    char suffix = get_suffix(bit/8);
    out << '\t' << "exts" << suffix << ' ' << r.m_no << ',' << r.m_no << '\n';
  }
  else {
    out << '\t' << "rlwinm " << r.m_no << ',' << r.m_no << ',';
    out << 0 << ',' << ((1 << bit)-1) << '\n';
  }
  x->store(r);
}

void gencode::common_longlong(const COMPILER::tac* tac)
{
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  reg r(reg::gpr,9);
  y->load(r);
  x->store(r);
}

void gencode::common_real(const COMPILER::tac* tac, bool signextend, int bit)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);

  reg fpr(reg::fpr,0);
  y->load(fpr);
  out << '\t' << "fctiwz " << fpr.m_no << ',' << fpr.m_no << '\n';
  tmp_dword->store(fpr);
  reg gpr(reg::gpr,9);
  tmp_dword->load(gpr);
  x->store(gpr);
}

void gencode::real_uint32(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);

  reg gpr(reg::gpr,9);
  y->load(gpr);
  tmp_dword->store(gpr,address::low);
  
  out << '\t' << "lis " << gpr.m_no << ',' << "0x4330" << '\n';
  tmp_dword->store(gpr,address::high);

  reg a(reg::fpr,0);
  tmp_dword->load(a);

  reg b(reg::fpr,13);
  if ( !double_0x100000000.get() ){
    const type* T = tac->x->m_type->varg();
        int size = T->size();
    double_0x100000000 = std::auto_ptr<mem>(new mem(new_label(),size));
  }
  double_0x100000000->load(b);

  out << '\t' << "fsub " << a.m_no << ',' << a.m_no << ',' << b.m_no << '\n';
  const type* T = tac->x->m_type;
  int size = T->size();
  if ( size == 4 )
    out << '\t' << "frsp " << a.m_no << ',' << a.m_no << '\n';

  x->store(a);
}

void gencode::real_sint64(const COMPILER::tac* tac, std::string func)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);

  reg r3(reg::gpr,3); y->load(r3,address::high);
  reg r4(reg::gpr,4); y->load(r4,address::low);
  out << '\t' << "bl " << func << '\n';
  
  reg f1(reg::fpr,1);
  x->store(f1);
}

void gencode::real_uint64(const COMPILER::tac* tac, std::string func, std::string fadd)
{
  using namespace std;
  using namespace COMPILER;

  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  reg r3(reg::gpr,3); y->load(r3,address::high);
  reg r4(reg::gpr,4); y->load(r4,address::low);
 
  out << '\t' << "li " << "5,0" << '\n';
  out << '\t' << "li " << "6,0" << '\n';
  out << '\t' << "bl " << "__cmpdi2" << '\n';
  out << '\t' << "cmpwi " << "7,3,1" << '\n';
  string label = new_label();
  out << '\t' << "blt 7," << label << '\n';

  y->load(r3,address::high);
  y->load(r4,address::low);
  out << '\t' << "bl " << func << '\n';
  reg f1(reg::fpr,1);
  x->store(f1);
  string label2 = new_label();
  out << '\t' << "bl " << label2 << '\n';

  out << label << ":\n";
  reg a(reg::gpr,9);
  y->load(a,address::high);
  out << '\t' << "andi. " << a.m_no << ',' << a.m_no << ",0\n";

  reg b(reg::gpr,10);
  y->load(b,address::high);
  out << '\t' << "srwi " << b.m_no << ',' << b.m_no << ",1\n";

  out << '\t' << "or " << r3.m_no << ',' << a.m_no << ',' << b.m_no << '\n';

  reg c(reg::gpr,11);
  y->load(b,address::low);
  out << '\t' << "rlwinm " << c.m_no << ',' << c.m_no << ",0,31,31\n";

  reg d(reg::gpr,12);
  y->load(d,address::high);
  out << '\t' << "slwi " << d.m_no << ',' << d.m_no << ",31\n";

  reg e(reg::gpr,13);
  y->load(e,address::low);
  out << '\t' << "srwi " << e.m_no << ',' << e.m_no << ',' << "1\n";

  reg f(reg::gpr,14);
  out << '\t' << "or " << f.m_no << ',' << d.m_no << ',' << e.m_no << '\n';

  out << '\t' << "or " << r4.m_no << ',' << c.m_no << ',' << f.m_no << '\n';

  out << '\t' << "bl " << func << '\n';
  out << '\t' << fadd << ' ' << f1.m_no << ',' << f1.m_no << ',' << f1.m_no;
  out << '\n';
  x->store(f1);
  out << label2 << ":\n";
}

void gencode::longlong_real(const COMPILER::tac* tac, std::string func)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);

  reg f1(reg::fpr,1);
  y->load(f1);
  out << '\t' << "bl" << '\t' << func << '\n';
  reg r3(reg::gpr,3);
  x->store(r3,address::high);
  reg r4(reg::gpr,4);
  x->store(r4,address::low);
}

void gencode::addr(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  reg r(reg::gpr,9);
  y->get(r);
  x->store(r);
}

void gencode::invladdr(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->z->m_type;
  T->scalar() ? invladdr_scalar(tac) : invladdr_aggregate(tac);
}

void gencode::invladdr_scalar(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->z->m_type;
  int size = T->size();
  size == 8 ? invladdr_dword(tac) : invladdr_notdword(tac, size);
}

void gencode::invladdr_notdword(const COMPILER::tac* tac, int size)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);

  reg ry(reg::gpr,9);
  y->load(ry);

  reg rz(reg::gpr,10);
  z->load(rz);

  char suffix = get_suffix(size);
  out << '\t' << "st" << suffix << ' ' << rz.m_no << ',';
  out << "0(" << ry.m_no << ")\n";
}

void gencode::invladdr_dword(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
 
  reg ry(reg::gpr,9);
  y->load(ry);

  reg rz(reg::fpr,1);
  z->load(rz);

  out << '\t' << "stfd " << rz.m_no << ',' << "0(" << ry.m_no << ")\n";
}

void gencode::invladdr_aggregate(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  address* z = getaddr(tac->z);
  reg r3(reg::gpr, 3);
  y->load(r3);
  const type* T = tac->z->m_type;
  int size = T->size();
  copy(0, z, size);
}

void gencode::invraddr(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  T->scalar() ? invraddr_scalar(tac) : invraddr_aggregate(tac);
}

void gencode::invraddr_scalar(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->x->m_type;
  int size = T->size();
  size == 8 ? invraddr_dword(tac) : invraddr_notdword(tac,size);
}

void gencode::invraddr_notdword(const COMPILER::tac* tac, int size)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);

  reg ry(reg::gpr,9);
  y->load(ry);
  
  char suffix = get_suffix(size);
  reg rx(reg::gpr,10);
  out << '\t' << 'l' << suffix << 'z' << ' ' << rx.m_no << ',';
  out << 0 << '(' << ry.m_no << ')' << '\n';
  x->store(rx);
}

void gencode::invraddr_dword(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);

  reg ry(reg::gpr,9);
  y->load(ry);

  reg rx(reg::fpr,1);
  out << '\t' << "lfd " << rx.m_no << ',' << "0(" << ry.m_no << ")\n";
  x->store(rx);
}

void gencode::invraddr_aggregate(const COMPILER::tac* tac)
{
        using namespace COMPILER;
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        reg r4(reg::gpr, 4);
        y->load(r4);
        const type* T = tac->x->m_type;
        int size = T->size();
        copy(x, 0, size);
}

void gencode::loff(const COMPILER::tac* tac)
{
        using namespace std;
        using namespace COMPILER;
        const type* T = tac->z->m_type;
        T->scalar() ? loff_scalar(tac) : loff_aggregate(tac);
}

void gencode::loff_scalar(const COMPILER::tac* tac)
{
        using namespace std;
        using namespace COMPILER;
        const type* T = tac->z->m_type;
        int size = T->size();
        size == 8 ? loff_dword(tac) : loff_notdword(tac, size);
}

void gencode::loff_notdword(const COMPILER::tac* tac, int size)
{
        using namespace COMPILER;
        {
                var* y = tac->y;
                if (y->isconstant())
                        return loff_notdword(tac, size, y->value());
        }
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        address* z = getaddr(tac->z);

        reg rx(reg::gpr, 9);
        x->get(rx);

        reg ry(reg::gpr, 10);
        y->load(ry);

        out << '\t' << "add " << rx.m_no << ',' << ry.m_no << ',' << ry.m_no << '\n';

        reg rz(reg::gpr, 11);
        z->load(rz);

        char suffix = get_suffix(size);
        out << '\t' << "st" << suffix << ' ' << rz.m_no << ',';
        out << "0(" << ry.m_no << ")\n";
}

void gencode::loff_notdword(const COMPILER::tac* tac, int size, int delta)
{
        using namespace std;
        using namespace COMPILER;
        address* x = getaddr(tac->x);
        address* z = getaddr(tac->z);

        reg rx(reg::gpr, 9);
        x->get(rx);
        reg rz(reg::gpr, 11);
        z->load(rz);

        char suffix = get_suffix(size);
        out << '\t' << "st" << suffix << ' ' << rz.m_no << ',';
        out << delta << '(' << rx.m_no << ")\n";
}

void gencode::loff_dword(const COMPILER::tac* tac)
{
        using namespace COMPILER;
        {
                var* y = tac->y;
                if (y->isconstant())
                        return loff_dword(tac, y->value());
        }
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        address* z = getaddr(tac->z);

        reg rx(reg::gpr, 9);
        x->get(rx);
        reg ry(reg::gpr, 10);
        y->load(ry);

        out << '\t' << "add " << rx.m_no << ',' << ry.m_no << ',' << ry.m_no << '\n';

        reg rz(reg::fpr, 1);
        z->load(rz);

        out << '\t' << "stfd " << rz.m_no << ',' << "0(" << ry.m_no << ")\n";
}

void gencode::loff_dword(const COMPILER::tac* tac, int delta)
{
        using namespace COMPILER;
        address* x = getaddr(tac->x);
        address* z = getaddr(tac->z);
        reg rx(reg::gpr, 9);
        x->get(rx);
        reg rz(reg::fpr, 1);
        z->load(rz);

        out << '\t' << "stfd " << rz.m_no << ',' << delta << '(' << rx.m_no << ")\n";
}

void gencode::loff_aggregate(const COMPILER::tac* tac)
{
        using namespace COMPILER;
        {
                var* y = tac->y;
                if (y->isconstant())
                        return loff_aggregate(tac, y->value());
        }
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        address* z = getaddr(tac->z);
        reg r3(reg::gpr, 3);
        x->get(r3);
        reg yy(reg::gpr, 9);
        y->load(yy);
        out << '\t' << "add " << r3.m_no << ',' << yy.m_no << ',' << r3.m_no << '\n';
        const type* T = tac->z->m_type;
        int size = T->size();
        copy(0, z, size);
}

void gencode::loff_aggregate(const COMPILER::tac* tac, int delta)
{
        using namespace COMPILER;
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        address* z = getaddr(tac->z);
        reg r3(reg::gpr, 3);
        x->get(r3);
        reg tmp(reg::gpr, 9);
        out << '\t' << "li ," << tmp.m_no << ',' << delta << '\n';
        out << '\t' << "add " << r3.m_no << ',' << tmp.m_no << ',' << r3.m_no << '\n';
        const type* T = tac->z->m_type;
        int size = T->size();
        copy(0, z, size);
}

void gencode::roff(const COMPILER::tac* tac)
{
        using namespace COMPILER;
        const type* T = tac->x->m_type;
        T->scalar() ? roff_scalar(tac) : roff_aggregate(tac);
}

void gencode::roff_scalar(const COMPILER::tac* tac)
{
        using namespace COMPILER;
        const type* T = tac->x->m_type;
        int size = T->size();
        size == 8 ? roff_dword(tac) : roff_notdword(tac, size);
}

void gencode::roff_notdword(const COMPILER::tac* tac, int size)
{
        using namespace COMPILER;
        {
                var* z = tac->z;
                if (z->isconstant())
                        return roff_notdword(tac, size, z->value());
        }

        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        address* z = getaddr(tac->z);

        reg ry(reg::gpr, 9);
        y->get(ry);
        reg rz(reg::gpr, 10);
        z->load(rz);

        out << '\t' << "add " << ry.m_no << ',' << rz.m_no << ',' << ry.m_no << '\n';

        char suffix = get_suffix(size);
        out << '\t' << 'l' << suffix << 'z' << ' ' << rz.m_no << ',';
        out << 0 << '(' << ry.m_no << ')' << '\n';
        x->store(rz);
}

void gencode::roff_notdword(const COMPILER::tac* tac, int size, int delta)
{
        using namespace COMPILER;

        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);

        reg ry(reg::gpr, 9);
        y->get(ry);

        reg rz(reg::gpr, 10);

        char suffix = get_suffix(size);
        out << '\t' << 'l' << suffix << 'z' << ' ' << rz.m_no << ',';
        out << delta << '(' << ry.m_no << ')' << '\n';
        x->store(rz);
}

void gencode::roff_dword(const COMPILER::tac* tac)
{
        using namespace COMPILER;
        {
                var* z = tac->z;
                if (z->isconstant())
                        return roff_dword(tac, z->value());
        }

        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        address* z = getaddr(tac->z);

        reg ry(reg::gpr, 9);
        y->get(ry);

        reg rz(reg::gpr, 10);
        z->load(rz);

        out << '\t' << "add " << ry.m_no << ',' << rz.m_no << ',' << ry.m_no << '\n';

        reg rx(reg::fpr, 1);
        out << '\t' << "lfd " << rx.m_no << ',' << "0(" << ry.m_no << ")\n";
        x->store(rx);
}

void gencode::roff_dword(const COMPILER::tac* tac, int delta)
{
        using namespace COMPILER;
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);

        reg ry(reg::gpr, 9);
        y->get(ry);

        reg rx(reg::fpr, 1);
        out << '\t' << "lfd " << rx.m_no << ',' << delta << '(' << ry.m_no << ")\n";
        x->store(rx);
}

void gencode::roff_aggregate(const COMPILER::tac* tac)
{
        using namespace COMPILER;
        {
                var* z = tac->z;
                if (z->isconstant())
                        return roff_aggregate(tac, z->value());
        }
        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);
        address* z = getaddr(tac->z);

        reg r4(reg::gpr, 4);
        y->get(r4);
        reg tmp(reg::gpr, 9);
        z->load(tmp);

        out << '\t' << "add " << r4.m_no << ',' << tmp.m_no << ',' << r4.m_no << '\n';

        const type* T = tac->x->m_type;
        int size = T->size();
        copy(x, 0, size);
}

void gencode::roff_aggregate(const COMPILER::tac* tac, int delta)
{
        using namespace COMPILER;

        address* x = getaddr(tac->x);
        address* y = getaddr(tac->y);

        reg r4(reg::gpr, 4);
        y->get(r4);

        out << '\t' << "addi " << r4.m_no << ',' << r4.m_no << ',' << delta << '\n';

        const type* T = tac->x->m_type;
        int size = T->size();
        copy(x, 0, size);
}

void gencode::param(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->y->m_type->promotion();
  if ( !T->scalar() )
    param_aggregate(tac);
  else if ( T->real() )
    param_real(tac);
  else {
    int size = T->size();
    size > 4 ? param_longlong(tac) : param_notlonglong(tac);
  }
}

void gencode::param_notlonglong(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  if ( m_gpr <= 10 ){
    reg r(reg::gpr,m_gpr);
    y->load(r);
    ++m_gpr;
  }
  else {
    reg r(reg::gpr,11);
    y->load(r);
    out << '\t' << "stw" << '\t' << r.m_no << ',' << m_offset << "(1)" << '\n';
    m_offset += 4;
  }
}

void gencode::param_real(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  if ( m_fpr <= 8 ){
    reg r(reg::fpr,m_fpr);
    y->load(r);
    ++m_fpr;
  }
  else {
    reg r(reg::fpr,9);
    y->load(r);
    const type* T = tac->y->m_type;
    int size = T->size();
    char suffix = size == 4 ? 's' : 'd';
    if ( size == 8 && (m_offset & 4) )
      m_offset += 4;
    out << '\t' << "stf" << suffix << '\t' << r.m_no << ',';
    out << m_offset << "(1)" << '\n';
    m_offset += size;
  }
}

void gencode::param_aggregate(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  map<var*, ::stack*>::const_iterator p =
    record_param.find(tac->y);
  assert(p != record_param.end());
  address* y = p->second;
  if ( m_gpr <= 10 ){
    reg r(reg::gpr,m_gpr);
    y->get(r);
    ++m_gpr;
  }
  else {
    reg r(reg::gpr,11);
    y->get(r);
    out << '\t' << "stw" << '\t' << r.m_no << ',' << m_offset << "(1)" << '\n';
    m_offset += 4;
  }
}

void gencode::param_longlong(const COMPILER::tac* tac)
{
  address* y = getaddr(tac->y);
  if ( !(m_gpr & 1) )
    ++m_gpr;

  if ( m_gpr <= 9 ){
    reg hi(reg::gpr,m_gpr);
    y->load(hi,address::high);
    reg lo(reg::gpr,m_gpr+1);
    y->load(lo,address::low);
    m_gpr += 2;
  }
  else {
    reg r(reg::fpr,9);
    y->load(r);
    if ( m_offset & 4 )
      m_offset += 4;
    out << '\t' << "stfd" << '\t' << r.m_no << ',';
    out << m_offset << "(1)" << '\n';
    m_offset += 8;
  }
}

int gencode::m_gpr = 3;

int gencode::m_fpr = 1;

int gencode::m_offset = 8;

void gencode::call(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  if ( m_fpr > 1 )
    out << '\t' << "creqv 6,6,6" << '\n';
  m_gpr = 3; m_fpr = 1; m_offset = 8; 
  if ( const var* ret = tac->x ){
    const type* T = ret->m_type;
    if ( !T->scalar() )
      call_aggregate(tac);
    else if ( T->real() )
      call_real(tac);
    else if ( T->size() > 4 )
      call_longlong(tac);
    else
      call_notlonglong(tac);
  }
  else call_void(tac);
}

void gencode::call_void(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  var* src1 = tac->y;
  const type* T = src1->m_type;
  address* y = getaddr(src1);
  if ( !T->scalar() ){
    mem* m = dynamic_cast<mem*>(y);
    out << '\t' << "bl " << m->label() << '\n';
  }
  else {
    reg r(reg::gpr,9);
    y->load(r);
    out << '\t' << "mtctr " << r.m_no << '\n';
    out << '\t' << "bctrl" << '\n';
  }
}

void gencode::call_notlonglong(const COMPILER::tac* tac)
{
  call_void(tac);
  address* x = getaddr(tac->x);
  reg r3(reg::gpr,3);
  x->store(r3);
}

void gencode::call_real(const COMPILER::tac* tac)
{
  call_void(tac);
  address* x = getaddr(tac->x);
  reg f1(reg::fpr,1);
  x->store(f1);
}

void gencode::call_longlong(const COMPILER::tac* tac)
{
  call_void(tac);
  address* x = getaddr(tac->x);
  reg r3(reg::gpr,3);
  x->store(r3,address::high);
  reg r4(reg::gpr,4);
  x->store(r4,address::low);
}

void gencode::call_aggregate(const COMPILER::tac* tac)
{
  address* x = getaddr(tac->x);
  reg r3(reg::gpr,3);
  x->get(r3);
  call_void(tac);
}

void gencode::prepare_aggregate_return(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const var* dest = tac->x;
  if ( !dest )
    return;
  const type* T = dest->m_type;
  if ( T->scalar() )
    return;
  ++m_gpr;
}

void gencode::_return(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  if ( tac->y ){
    const type* T = tac->y->m_type;
    if ( !T->scalar() )
      _return_aggregate(tac);
    else if ( T->real() )
      _return_real(tac);
    else if ( T->size() > 4 )
      _return_longlong(tac);
    else
      _return_notlonglong(tac);
  }

  if ( m_last != tac ){
    out << '\t' << "b" << '\t' << function_exit.m_label << '\n';
    function_exit.m_ref = true;
  }
}

void gencode::_return_notlonglong(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  address* y = getaddr(tac->y);
  reg r3(reg::gpr,3);
  y->load(r3);
}

void gencode::_return_real(const COMPILER::tac* tac)
{
  address* y = getaddr(tac->y);
  reg r(reg::fpr,1);
  y->load(r);
}

void gencode::_return_longlong(const COMPILER::tac* tac)
{
  address* y = getaddr(tac->y);
  reg r3(reg::gpr,3);
  y->load(r3,address::high);
  reg r4(reg::gpr,4);
  y->load(r4,address::low);
}

void gencode::_return_aggregate(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  stack ret(parameter::m_hidden,4);
  reg r3(reg::gpr,3);
  ret.load(r3);
  address* y = getaddr(tac->y);
  const type* T = tac->y->m_type;
  int size = T->size();
  copy(0, y, size);
}

const COMPILER::tac* gencode::m_last;

void gencode::ifgoto(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const type* T = tac->y->m_type;
  if ( T->real() )
    ifgoto_real(tac);
  else if ( T->size() > 4 )
    ifgoto_longlong(tac);
  else
    ifgoto_notlonglong(tac);
}

struct gencode::ifgoto_table : std::map<COMPILER::goto3ac::op, std::string> {
  ifgoto_table()
  {
    using namespace COMPILER;
    (*this)[goto3ac::EQ] = "beq";
    (*this)[goto3ac::NE] = "bne";
    (*this)[goto3ac::LT]  = "blt";
    (*this)[goto3ac::GT]  = "bgt";
    (*this)[goto3ac::LE] = "ble";
    (*this)[goto3ac::GE] = "bge";
  }
};

gencode::ifgoto_table gencode::m_ifgoto_table;

void gencode::ifgoto_notlonglong(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;

  address* x = getaddr(tac->y);
  address* y = getaddr(tac->z);
  reg rx(reg::gpr,9);  x->load(rx);
  reg ry(reg::gpr,10); y->load(ry);

  out << '\t' << "cmp";
  const type* T = tac->y->m_type;
  if ( !T->_signed() )
    out << 'l';
  out << "w 7," << rx.m_no << ',' << ry.m_no << '\n';
  const goto3ac* ptr = static_cast<const goto3ac*>(tac);
  out << '\t' << m_ifgoto_table[ptr->m_op] << " 7," << '.' << func_label << ptr->m_to << '\n';
}

struct ifgoto_real_type {
  std::string m_op;
  int m_cror;
  ifgoto_real_type(std::string op = "", int cror = 0)
    : m_op(op), m_cror(cror) {}
};

struct gencode::ifgoto_real_table
  : public std::map<COMPILER::goto3ac::op, ifgoto_real_type> {
public:
  ifgoto_real_table()
  {
        using namespace COMPILER;
    (*this)[goto3ac::EQ] = ifgoto_real_type("bne",0);
    (*this)[goto3ac::NE] = ifgoto_real_type("beq",0);
    (*this)[goto3ac::LT]  = ifgoto_real_type("beq",29);
    (*this)[goto3ac::GT]  = ifgoto_real_type("beq",28);
    (*this)[goto3ac::LE] = ifgoto_real_type("bgt",0);
    (*this)[goto3ac::GE] = ifgoto_real_type("blt",0);
  }
};

gencode::ifgoto_real_table gencode::m_ifgoto_real_table;

void gencode::ifgoto_real(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;

  const goto3ac* ptr = static_cast<const goto3ac*>(tac);
  goto3ac::op op = ptr->m_op;
  map<goto3ac::op,ifgoto_real_type>::const_iterator p =
    m_ifgoto_real_table.find(op);
  assert(p != m_ifgoto_real_table.end());
  const ifgoto_real_type& irt = p->second;

  address* x = getaddr(tac->y);
  address* y = getaddr(tac->z);
  reg rx(reg::fpr,0); x->load(rx);
  reg ry(reg::fpr,1); y->load(ry);
  out << '\t' << "fcmpu 7," << rx.m_no << ',' << ry.m_no << '\n';

  if (irt.m_cror )
    out << '\t' << "cror 30," << irt.m_cror << ",30" << '\n';

  string stay = new_label();
  out << '\t' << irt.m_op << " 7," << stay << '\n';
  out << '\t' << "b " << '.' << func_label << ptr->m_to << '\n';
  out << stay << ":\n";
}

struct ifgoto_longlong_type {
  struct jmp {
    std::string m_op;
    bool m_stay;
    jmp(std::string op = "", bool stay = false)
      : m_op(op), m_stay(stay) {}
  };
  jmp m_1st;
  jmp m_2nd;
  jmp m_3rd;
  bool m_swap;
  bool m_last;
  bool m_redundant_stay;
  ifgoto_longlong_type(jmp _1st = jmp(), jmp _2nd = jmp(), jmp _3rd = jmp(),
                       bool flag = false, bool last = false,
                       bool redundant_stay = false)
    : m_1st(_1st), m_2nd(_2nd), m_3rd(_3rd),
      m_swap(flag), m_last(last), m_redundant_stay(redundant_stay) {}
};

struct gencode::ifgoto_longlong_table
  : public std::map<COMPILER::goto3ac::op, ifgoto_longlong_type> {
public:
  ifgoto_longlong_table()
  {
        using namespace COMPILER;
    typedef ifgoto_longlong_type::jmp jmp;
    (*this)[goto3ac::EQ] = ifgoto_longlong_type(jmp("bne",true),
                                               jmp(),
                                               jmp("bne",true),
                                               false,
                                               true,
                                               false);
    (*this)[goto3ac::NE] = ifgoto_longlong_type(jmp("bne",false),
                                               jmp(),
                                               jmp("bne",false),
                                               false,
                                               false,
                                               true);
    (*this)[goto3ac::LT] = ifgoto_longlong_type(jmp("bgt",false),
                                               jmp("bne",true),
                                               jmp("bgt",false),
                                               true,
                                               false,
                                               false);
    (*this)[goto3ac::GT] = ifgoto_longlong_type(jmp("bgt",false),
                                               jmp("bne",true),
                                               jmp("bgt",false),
                                               false,
                                               false,
                                               false);
    (*this)[goto3ac::LE] = ifgoto_longlong_type(jmp("bgt",true),
                                               jmp("bne",false),
                                               jmp("bgt",true),
                                               false,
                                               true,
                                               false);
    (*this)[goto3ac::GE] = ifgoto_longlong_type(jmp("bgt",true),
                                               jmp("bne",false),
                                               jmp("bgt",true),
                                               true,
                                            true,
                                               false);
  }
};

gencode::ifgoto_longlong_table gencode::m_ifgoto_longlong_table;

void gencode::ifgoto_longlong(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;

  const goto3ac* ptr = static_cast<const goto3ac*>(tac);
  goto3ac::op op = ptr->m_op;
  map<goto3ac::op,ifgoto_longlong_type>::const_iterator p =
    m_ifgoto_longlong_table.find(op);
  assert(p != m_ifgoto_longlong_table.end());
  const ifgoto_longlong_type& pattern = p->second;

  const type* T = tac->y->m_type;
  string stay = new_label();
  ostringstream os;
  os << '.' << func_label << ptr->m_to;
  string label = os.str();

  address* x = getaddr(tac->y);
  address* y = getaddr(tac->z);
  reg rx(reg::gpr,9);
  reg ry(reg::gpr,10);

  x->load(rx,address::high);
  y->load(ry,address::high);
  out << '\t' << "cmp";
  if ( T->_signed() )
    out << 'l';
  out << "w 7,";
  if ( pattern.m_swap )
    out << ry.m_no << ',' << rx.m_no;
  else
    out << rx.m_no << ',' << ry.m_no;
  out << '\n';
  out << '\t' << pattern.m_1st.m_op << " 7,";
  if ( pattern.m_1st.m_stay )
    out << stay;
  else
    out << label;
  out << '\n';

  if ( !pattern.m_2nd.m_op.empty() ){
  out << '\t' << "cmp";
  if ( T->_signed() )
    out << 'l';
  out << "w 7,";
  if ( pattern.m_swap )
    out << ry.m_no << ',' << rx.m_no;
  else
    out << rx.m_no << ',' << ry.m_no;
  out << '\n';
  out << '\t' << pattern.m_2nd.m_op << " 7,";
  if ( pattern.m_2nd.m_stay )
    out << stay;
  else
    out << label;
  out << '\n';
  }

  x->load(rx,address::low);
  y->load(ry,address::low);
  out << '\t' << "cmplw 7,";
  if ( pattern.m_swap )
    out << ry.m_no << ',' << rx.m_no;
  else
    out << rx.m_no << ',' << ry.m_no;
  out << '\n';
  out << '\t' << pattern.m_3rd.m_op << " 7,";
  if ( pattern.m_1st.m_stay )
    out << stay;
  else
    out << label;
  out << '\n';

  if ( pattern.m_last )
    out << '\t' << "b " << label << '\n';

  if ( !pattern.m_redundant_stay )
    out << stay << ":\n";
}

void gencode::_goto(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const goto3ac* ptr = static_cast<const goto3ac*>(tac);
  if (ptr->m_op == goto3ac::NONE)
          out << '\t' << "b " << '.' << func_label << ptr->m_to << '\n';
  else
          ifgoto(tac);
}

void gencode::to(const COMPILER::tac* tac)
{
        out << '.' << func_label << tac << '\n';
}

void gencode::alloc(const COMPILER::tac* tac)
{
  stack sp_old(0,4);
  reg a(reg::gpr,9);
  sp_old.load(a);

  reg b(reg::gpr,0);
  address* y = getaddr(tac->y);
  y->load(b);
  out << '\t' << "neg " << b.m_no << ',' << b.m_no << '\n';
  
  out << '\t' << "stwux " << a.m_no << ",1," << b.m_no << '\n';
  int c = 8 + stack_layout.m_fun_arg;
  out << '\t' << "addi "  << a.m_no << ",1," << c << '\n';

  address* x = getaddr(tac->x);
  alloced_addr* p = static_cast<alloced_addr*>(x);
  p->set(a);
}

void gencode::dealloc(const COMPILER::tac* tac)
{
        // nothing to be done
}

void gencode::asm_(const COMPILER::tac* tac)
{
  using namespace COMPILER;
  const asm3ac* p = static_cast<const asm3ac*>(tac);
  out << '\t' << p->m_inst << '\n';
}

void gencode::_va_start(const COMPILER::tac* tac)
{
  _va_start_counter(tac);
  {
    int offset = current_varg.m_offset.m_gpr;
    stack* ptr = current_varg.m_pointer.m_gpr;
    _va_start_pointer(offset,ptr);
  }
  {
    int offset = current_varg.m_offset.m_fpr;
    stack* ptr = current_varg.m_pointer.m_fpr;
    _va_start_pointer(offset,ptr);
  }

  address* x = getaddr(tac->x);
  address* y = getaddr(tac->y);
  reg r(reg::gpr,9);
  y->get(r);
  x->store(r);
}

void gencode::_va_start_counter(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const type* T = tac->y->m_type;
  ::stack* gpr = current_varg.m_counter.m_gpr;
  ::stack* fpr = current_varg.m_counter.m_fpr;
  pair<::stack*, ::stack*> counter = T->real() ?
    make_pair(fpr,gpr) :
    make_pair(gpr,fpr) ;

  int size = T->size();
  int n = (T->integer() && size == 8) ? 2 : 1;

  reg r(reg::gpr,9);
  out << '\t' << "li " << r.m_no << ',' << n << '\n';
  counter.first->store(r);
  out << '\t' << "li " << r.m_no << ',' << 0 << '\n';
  counter.second->store(r);
}

void gencode::_va_start_pointer(int offset, stack* ptr)
{
  reg r(reg::gpr,9);
  out << '\t' << "li " << r.m_no << ',' << offset << '\n';
  out << '\t' << "add " << r.m_no << ',' << r.m_no << ',' << 31 << '\n';
  ptr->store(r);
}

void gencode::_va_arg(const COMPILER::tac* tac)
{
  using namespace std;
  using namespace COMPILER;
  const va_arg3ac* ptr = static_cast<const va_arg3ac*>(tac);
  const type* T = ptr->m_type;
  
  ::stack* counter = T->real() ? current_varg.m_counter.m_fpr :
    current_varg.m_counter.m_gpr;
  ::stack* pointer = T->real() ? current_varg.m_pointer.m_fpr :
    current_varg.m_pointer.m_gpr;
  int size = T->size();
  int inc = (T->integer() && size == 8) ? 2 : 1;
  address* y = getaddr(tac->y);

  reg ra(reg::gpr,9);
  counter->load(ra);
  out << '\t' << "cmpwi 7," << ra.m_no << ',' << 8 << '\n';
  string judge = new_label();
  out << '\t' << "beq 7," << judge << '\n';
  string just_update = new_label();
  out << '\t' << "bgt 7," << just_update << '\n';

  out << '\t' << "addi " << ra.m_no << ',' << ra.m_no << ',' << inc << '\n';
  reg rb(reg::gpr,10);
  if ( inc == 2 )
    _va_arg_adjust_longlong(ra,rb);
  counter->store(ra);
  pointer->load(ra);
  y->store(ra);
  out << '\t' << "addi " << ra.m_no << ',' << ra.m_no << ',' << size << '\n';
  pointer->store(ra);
  string just_load = new_label();
  out << '\t' << "b " << just_load << '\n';

  out << judge << ":\n";
  ::stack* opposite_counter = T->real() ?
    current_varg.m_counter.m_gpr :
    current_varg.m_counter.m_fpr ;
  opposite_counter->load(rb);
  out << '\t' << "cmpw 7," << ra.m_no << ',' << rb.m_no << '\n';
  string second_visitor = new_label();
  out << '\t' << "ble 7," << second_visitor << '\n';
  out << '\t' << "addi " << ra.m_no << ',' << ra.m_no << ',' << inc << '\n';
  counter->store(ra);
  out << '\t' << "addi " << ra.m_no << ',' << 31 << ',';
  out << stack_layout.m_size + 8 << '\n';
  y->store(ra);
  out << '\t' << "addi " << ra.m_no << ',' << ra.m_no << ',' << size << '\n';
  pointer->store(ra);
  out << '\t' << "b " << just_load << '\n';

  out << second_visitor << ":\n";
  out << '\t' << "addi " << ra.m_no << ',' << ra.m_no << ',' << inc << '\n';
  counter->store(ra);
  ::stack* opposite_pointer = T->real() ?
    current_varg.m_pointer.m_gpr :
    current_varg.m_pointer.m_fpr ;
  opposite_pointer->load(ra);
  y->store(ra);
  out << '\t' << "addi " << ra.m_no << ',' << ra.m_no << ',' << size << '\n';
  pointer->store(ra);
  opposite_pointer->store(ra);
  out << '\t' << "b " << just_load << '\n';

  out << just_update << ":\n";
  out << '\t' << "addi " << ra.m_no << ',' << ra.m_no << ',' << inc << '\n';
  if ( inc == 2 )
    _va_arg_adjust_longlong(ra,rb);
  counter->store(ra);
  pointer->load(ra);
  y->store(ra);
  out << '\t' << "addi " << ra.m_no << ',' << ra.m_no << ',' << size << '\n';
  pointer->store(ra);

  out << just_load << ":\n";
  y->load(ra);
  address* x = getaddr(tac->x);
  if ( size == 8 ){
    reg rc(reg::fpr,0);
    out << '\t' << "lfd " << rc.m_no << ", 0(" << ra.m_no << ")\n";
    x->store(rc);
  }
  else {
    out << '\t' << "lwz " << ra.m_no << ", 0(" << ra.m_no << ")\n";
    x->store(ra);
  }
}

void gencode::_va_arg_adjust_longlong(const reg& ra, const reg& rb)
{
  using namespace std;
  string label = new_label();
  out << '\t' << "li " << rb.m_no << ',' << 1 << '\n';
  out << '\t' << "and " << rb.m_no << ',' << ra.m_no << ',' << rb.m_no;
  out << '\n';
  out << '\t' << "cmpwi 7, " << rb.m_no << ',' << 1 << '\n';
  out << '\t' << "bne 7, " << label << '\n';
  out << '\t' << "addi " << ra.m_no << ',' << ra.m_no << ',' << 1 << '\n';
  ::stack* pointer = current_varg.m_pointer.m_gpr;
  pointer->load(rb);
  out << '\t' << "addi " << rb.m_no << ',' << rb.m_no << ',' << 4 << '\n';
  pointer->store(rb);
  out << label << ":\n";
}

void gencode::_va_end(const COMPILER::tac*)
{
        // nothing to be done.
}
