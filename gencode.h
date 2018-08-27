#ifndef _GENCODE_H_
#define _GENCODE_H_

class gencode {
  struct table;
  static table m_table;
  friend struct table;
  static void assign(const COMPILER::tac*);
  static void assign_scalar(const COMPILER::tac*);
  static void assign_aggregate(const COMPILER::tac*);
  static void add(const COMPILER::tac*);
  static void sub(const COMPILER::tac*);
  static void mul(const COMPILER::tac*);
  static void mul_longlong(const COMPILER::tac*);
  static void div(const COMPILER::tac*);
  static void mod(const COMPILER::tac*);
  static void lsh(const COMPILER::tac*);
  static void rsh(const COMPILER::tac*);
  static void runtime_longlong_shift(const COMPILER::tac*, std::string);
  static void _and(const COMPILER::tac*);
  static void _xor(const COMPILER::tac*);
  static void _or(const COMPILER::tac*);
  static void binop_notlonglong(const COMPILER::tac*, std::string);
  static void binop_real(const COMPILER::tac*, std::string);
  static void binop_longlong(const COMPILER::tac*, std::string, std::string);
  static void runtime_longlong(const COMPILER::tac*, std::string);
  static void uminus(const COMPILER::tac*);
  static void uminus_notlonglong(const COMPILER::tac*);
  static void uminus_real(const COMPILER::tac*);
  static void uminus_longlong(const COMPILER::tac*);
  static void tilde(const COMPILER::tac*);
  static void tc(const COMPILER::tac*);
  static int tc_id(const COMPILER::type*);
  struct tc_table;
  static tc_table m_tc_table;
  friend struct tc_table;
  static void sint08_sint08(const COMPILER::tac*);
  static void sint08_uint08(const COMPILER::tac*);
  static void sint08_sint16(const COMPILER::tac*);
  static void sint08_uint16(const COMPILER::tac*);
  static void sint08_sint32(const COMPILER::tac*);
  static void sint08_uint32(const COMPILER::tac*);
  static void sint08_sint64(const COMPILER::tac*);
  static void sint08_uint64(const COMPILER::tac*);
  static void sint08_single(const COMPILER::tac*);
  static void sint08_double(const COMPILER::tac*);
  static void uint08_sint08(const COMPILER::tac*);
  static void uint08_sint16(const COMPILER::tac*);
  static void uint08_uint16(const COMPILER::tac*);
  static void uint08_sint32(const COMPILER::tac*);
  static void uint08_uint32(const COMPILER::tac*);
  static void uint08_sint64(const COMPILER::tac*);
  static void uint08_uint64(const COMPILER::tac*);
  static void uint08_single(const COMPILER::tac*);
  static void uint08_double(const COMPILER::tac*);
  static void sint16_sint08(const COMPILER::tac*);
  static void sint16_uint08(const COMPILER::tac*);
  static void sint16_uint16(const COMPILER::tac*);
  static void sint16_sint32(const COMPILER::tac*);
  static void sint16_uint32(const COMPILER::tac*);
  static void sint16_sint64(const COMPILER::tac*);
  static void sint16_uint64(const COMPILER::tac*);
  static void sint16_single(const COMPILER::tac*);
  static void sint16_double(const COMPILER::tac*);
  static void uint16_sint08(const COMPILER::tac*);
  static void uint16_uint08(const COMPILER::tac*);
  static void uint16_sint16(const COMPILER::tac*);
  static void uint16_sint32(const COMPILER::tac*);
  static void uint16_uint32(const COMPILER::tac*);
  static void uint16_sint64(const COMPILER::tac*);
  static void uint16_uint64(const COMPILER::tac*);
  static void uint16_single(const COMPILER::tac*);
  static void uint16_double(const COMPILER::tac*);
  static void sint32_sint08(const COMPILER::tac*);
  static void sint32_uint08(const COMPILER::tac*);
  static void sint32_sint16(const COMPILER::tac*);
  static void sint32_uint16(const COMPILER::tac*);
  static void sint32_sint32(const COMPILER::tac*);
  static void sint32_uint32(const COMPILER::tac*);
  static void sint32_sint64(const COMPILER::tac*);
  static void sint32_uint64(const COMPILER::tac*);
  static void sint32_single(const COMPILER::tac*);
  static void sint32_double(const COMPILER::tac*);
  static void uint32_sint08(const COMPILER::tac*);
  static void uint32_uint08(const COMPILER::tac*);
  static void uint32_sint16(const COMPILER::tac*);
  static void uint32_uint16(const COMPILER::tac*);
  static void uint32_sint32(const COMPILER::tac*);
  static void uint32_uint32(const COMPILER::tac*);
  static void uint32_sint64(const COMPILER::tac*);
  static void uint32_uint64(const COMPILER::tac*);
  static void uint32_single(const COMPILER::tac*);
  static void uint32_double(const COMPILER::tac*);
  static void uint32_real(const COMPILER::tac*);
  static void sint64_sint08(const COMPILER::tac*);
  static void sint64_uint08(const COMPILER::tac*);
  static void sint64_sint16(const COMPILER::tac*);
  static void sint64_uint16(const COMPILER::tac*);
  static void sint64_sint32(const COMPILER::tac*);
  static void sint64_uint32(const COMPILER::tac*);
  static void sint64_uint64(const COMPILER::tac*);
  static void sint64_single(const COMPILER::tac*);
  static void sint64_double(const COMPILER::tac*);
  static void uint64_sint08(const COMPILER::tac*);
  static void uint64_uint08(const COMPILER::tac*);
  static void uint64_sint16(const COMPILER::tac*);
  static void uint64_uint16(const COMPILER::tac*);
  static void uint64_sint32(const COMPILER::tac*);
  static void uint64_uint32(const COMPILER::tac*);
  static void uint64_sint64(const COMPILER::tac*);
  static void uint64_single(const COMPILER::tac*);
  static void uint64_double(const COMPILER::tac*);
  static void extend64(const COMPILER::tac*, bool, int);
  static void single_sint08(const COMPILER::tac*);
  static void single_uint08(const COMPILER::tac*);
  static void single_sint16(const COMPILER::tac*);
  static void single_uint16(const COMPILER::tac*);
  static void single_sint32(const COMPILER::tac*);
  static void single_uint32(const COMPILER::tac*);
  static void single_sint64(const COMPILER::tac*);
  static void single_uint64(const COMPILER::tac*);
  static void single_double(const COMPILER::tac*);
  static void double_sint08(const COMPILER::tac*);
  static void double_uint08(const COMPILER::tac*);
  static void double_sint16(const COMPILER::tac*);
  static void double_uint16(const COMPILER::tac*);
  static void double_sint32(const COMPILER::tac*);
  static void double_uint32(const COMPILER::tac*);
  static void double_sint64(const COMPILER::tac*);
  static void double_uint64(const COMPILER::tac*);
  static void double_single(const COMPILER::tac*);
  static void double_double(const COMPILER::tac*);
  static void extend(const COMPILER::tac*, bool, int);
  static void common_longlong(const COMPILER::tac*);
  static void common_real(const COMPILER::tac*, bool, int);
  static void real_common(const COMPILER::tac*, bool, int);
  static void real_uint32(const COMPILER::tac*);
  static void real_sint64(const COMPILER::tac*, std::string);
  static void real_uint64(const COMPILER::tac*, std::string, std::string);
  static void longlong_real(const COMPILER::tac*, std::string);
  static void addr(const COMPILER::tac*);
  static void invladdr(const COMPILER::tac*);
  static void invladdr_scalar(const COMPILER::tac*);
  static void invladdr_notdword(const COMPILER::tac*, int);
  static void invladdr_dword(const COMPILER::tac*);
  static void invladdr_aggregate(const COMPILER::tac*);
  static void invraddr(const COMPILER::tac*);
  static void invraddr_scalar(const COMPILER::tac*);
  static void invraddr_notdword(const COMPILER::tac*, int);
  static void invraddr_dword(const COMPILER::tac*);
  static void invraddr_aggregate(const COMPILER::tac*);
  static void loff(const COMPILER::tac*);
  static void loff_scalar(const COMPILER::tac*);
  static void loff_notdword(const COMPILER::tac*, int size);
  static void loff_notdword(const COMPILER::tac*, int size, int delta);
  static void loff_dword(const COMPILER::tac*);
  static void loff_dword(const COMPILER::tac*, int delta);
  static void loff_aggregate(const COMPILER::tac*);
  static void loff_aggregate(const COMPILER::tac*, int delta);
  static void roff(const COMPILER::tac*);
  static void roff_scalar(const COMPILER::tac*);
  static void roff_notdword(const COMPILER::tac*, int size);
  static void roff_notdword(const COMPILER::tac*, int size, int delta);
  static void roff_dword(const COMPILER::tac*);
  static void roff_dword(const COMPILER::tac*, int delta);
  static void roff_aggregate(const COMPILER::tac*);
  static void roff_aggregate(const COMPILER::tac*, int delta);
  static void param(const COMPILER::tac*);
  static void param_notlonglong(const COMPILER::tac*);
  static void param_real(const COMPILER::tac*);
  static void param_aggregate(const COMPILER::tac*);
  static void param_longlong(const COMPILER::tac*);
  static int m_gpr;
  static int m_fpr;
  static int m_offset;
  static void call(const COMPILER::tac*);
  static void call_void(const COMPILER::tac*);
  static void call_notlonglong(const COMPILER::tac*);
  static void call_real(const COMPILER::tac*);
  static void call_longlong(const COMPILER::tac*);
  static void call_aggregate(const COMPILER::tac*);
  static void prepare_aggregate_return(const COMPILER::tac*);
  static void _return(const COMPILER::tac*);
  static void _return_notlonglong(const COMPILER::tac*);
  static void _return_real(const COMPILER::tac*);
  static void _return_longlong(const COMPILER::tac*);
  static void _return_aggregate(const COMPILER::tac*);
  static const COMPILER::tac* m_last;
  static void ifgoto(const COMPILER::tac*);
  static void ifgoto_notlonglong(const COMPILER::tac*);
  static void ifgoto_real(const COMPILER::tac*);
  static void ifgoto_longlong(const COMPILER::tac*);
  struct ifgoto_table;
  static ifgoto_table m_ifgoto_table;
  struct ifgoto_real_table;
  static ifgoto_real_table m_ifgoto_real_table;
  struct ifgoto_longlong_table;
  static ifgoto_longlong_table m_ifgoto_longlong_table;
  static void if_real_eq(const COMPILER::tac*);
  static void if_real_ne(const COMPILER::tac*);
  static void if_real_lt(const COMPILER::tac*);
  static void if_real_gt(const COMPILER::tac*);
  static void if_real_le(const COMPILER::tac*);
  static void if_real_ge(const COMPILER::tac*);
  static void _goto(const COMPILER::tac*);
  static void to(const COMPILER::tac*);
  static void alloc(const COMPILER::tac*);
  static void dealloc(const COMPILER::tac*);
  static void asm_(const COMPILER::tac*);
  static void _va_start(const COMPILER::tac*);
  static void _va_start_counter(const COMPILER::tac*);
  static void _va_start_pointer(int, stack*);
  static void _va_arg(const COMPILER::tac*);
  static void _va_arg_adjust_longlong(const reg&, const reg&);
  static void _va_end(const COMPILER::tac*);

  const std::vector<COMPILER::tac*>& m_v3ac;
  int m_counter;
  bool m_record_stuff;
public:
  gencode(const std::vector<COMPILER::tac*>& v3ac)
    : m_v3ac(v3ac), m_counter(-1), m_record_stuff(false)
    { m_last = v3ac.back(); }
  void operator()(const COMPILER::tac*);
};

#endif // _GENCODE_H_