#include "stdafx.h"

#include "c_core.h"

#define COMPILER c_compiler

#include "ppc.h"

void output_data(const std::pair<std::string, std::vector<COMPILER::usr*> >&);

void genobj(const COMPILER::scope* tree)
{
        using namespace std;
        using namespace COMPILER;
        const map<string, vector<usr*> >& usrs = tree->m_usrs;
        for_each(usrs.begin(), usrs.end(), output_data);
        const vector<scope*>& ch = tree->m_children;
        for_each(ch.begin(), ch.end(), genobj);
}

void output_data2(COMPILER::usr*);

void output_data(const std::pair<std::string, std::vector<COMPILER::usr*> >& p)
{
        using namespace std;
        using namespace COMPILER;
        const vector<usr*>& vec = p.second;
        for_each(vec.begin(), vec.end(), output_data2);
}

void constant_data(COMPILER::usr* entry);

void string_literal_data(COMPILER::usr* entry);

int initial(int offset, std::pair<int, COMPILER::var*>);

int static_counter;

void output_data2(COMPILER::usr* entry)
{
        using namespace std;
        using namespace COMPILER;
        map<const var*, address*>::const_iterator p = address_descriptor.find(entry);
        if (p != address_descriptor.end())
                return;
        usr::flag flag = entry->m_flag;
        if (flag & usr::TYPEDEF)
                return;

        const type* T = entry->m_type;
        string name = entry->m_name;

        usr::flag mask = usr::flag(usr::EXTERN|usr::FUNCTION);
        if (flag & mask) {
                int size = T->size();
                address_descriptor[entry] = new mem(name, size);
                return;
        }

        if (entry->isconstant())
                return constant_data(entry);

        if (string_literal(entry))
                return string_literal_data(entry);

        if (!(flag & usr::STATIC) && !is_top(entry->m_scope))
                return;

        output_section(T->modifiable() ? ram : rom);

        if (flag & usr::STATIC) {
                if (!is_top(entry->m_scope)) {
                        ostringstream os;
                        os << '.' << ++static_counter;
                        name += os.str();
                }
        }
        else 
                out << '\t' << ".global" << '\t' << name << '\n';
        out << '\t' << ".align" << '\t' << T->align() << '\n';
        out << name << ':' << '\n';

        int size = T->size();

        if (flag & usr::WITH_INI) {
                with_initial* wi = static_cast<with_initial*>(entry);
                map<int, var*>& value = wi->m_value;
                if (int n = size - accumulate(value.begin(), value.end(), 0, initial))
                        out << '\t' << ".space" << n << '\n';
        }

        int n = size < 16 ? 16 : size + 16;
        out << '\t' << ".comm" << '\t' << name << ", " << n << " # " << size << '\n';

        address_descriptor[entry] = new mem(name, size);
}

void constant_data(COMPILER::usr* entry)
{
        using namespace std;
        using namespace COMPILER;
        const type* T = entry->m_type;
        int size = T->size();
        if (T->real() || size > 4) {
                output_section(rom);
                out << '\t' << ".align" << '\t' << T->align() << '\n';
                string label = new_label();
                out << label << ":\n";
                initial(0, make_pair(0,entry));
                address_descriptor[entry] = new mem(label, size);
        }
        else
                address_descriptor[entry] = new imm(entry);
}

void string_literal_data(COMPILER::usr* entry)
{
        using namespace std;
        using namespace COMPILER;
        output_section(rom);
        string label = new_label();
        out << label << ":\n";
        string name = entry->m_name;
        name.erase(name.size() - 1);
        name += "\\0\"";
        out << '\t' << ".ascii" << '\t' << name << '\n';
        const type* T = entry->m_type;
        int size = T->size();
        address_descriptor[entry] = new mem(label, size);
}

int initial_real(int offset, COMPILER::usr* entry);

int initial_notreal(int offset, COMPILER::usr* entry);

int initial(int offset, std::pair<int, COMPILER::var*> x)
{
        using namespace std;
        using namespace COMPILER;
        int specified = x.first;
        if (int n = specified - offset ) {
                out << '\t' << ".space" << '\t' << n << '\n';
                offset = specified;
        }
        var* v = x.second;
        usr* entry = v->usr_cast();
        const type* T = entry->m_type;
        if (T->real())
                return initial_real(offset, entry);
        if (T->scalar())
                return initial_notreal(offset, entry);
        assert(string_literal(entry));
        string name = entry->m_name;
        name.erase(name.size() - 1);
        name += "\\0\"";
        out << '\t' << ".ascii" << '\t' << name << '\n';
        return offset + name.size();
}

int initial_notreal(int offset, COMPILER::usr* entry)
{
        using namespace std;
        using namespace COMPILER;
        const type* T = entry->m_type;
        int size = T->size();
        int n = entry->value();
        switch (size) {
        case 1:
                out << '\t' << ".byte" << '\t' << n << '\n';
                return offset + 1;
        case 2:
                out << '\t' << ".short" << '\t' << n << '\n';
                return offset + 2;
        case 4:
                out << '\t' << ".long" << '\t' << n << '\n';
                return offset + 4;
        }

        union {
                __int64 ll;
                int i[2];
        } tmp;
        tmp.ll = strtoull(entry->m_name.c_str(), 0, 0);
        int i = 1;
        if (*(char*)&i)
                swap(tmp.i[0], tmp.i[1]);
        out << '\t' << ".long" << '\t' << tmp.i[0] << '\n';
        out << '\t' << ".long" << '\t' << tmp.i[1] << '\n';
        return offset + 8;
}

int initial_real(int offset, COMPILER::usr* entry)
{
        using namespace std;
        using namespace COMPILER;
        const type* T = entry->m_type;
        int size = T->size();
        if (size == 4) {
                union {
                        float f;
                        int i;
                } tmp;
                tmp.f = atof(entry->m_name.c_str());
                std::ostringstream os;
                out << '\t' << ".long" << '\t' << tmp.i;
        }
        else {
                union {
                        double d;
                        int i[2];
                } tmp;
                tmp.d = atof(entry->m_name.c_str());
                int i = 1;
                if (*(char*)&i)
                        swap(tmp.i[0], tmp.i[1]);
                out << '\t' << ".long" << '\t' << tmp.i[0] << '\n';
                out << '\t' << ".long" << '\t' << tmp.i[1] << '\n';
        }
        return offset + size;
}


#ifdef CXX_GENERATOR
struct cxxbuiltin_table : std::map<std::string, std::string> {
  cxxbuiltin_table()
  {
    (*this)["new"] = "_Znwj";
    (*this)["delete"] = "_ZdlPv";

    (*this)["new.a"] = "_Znaj";
    (*this)["delete.a"] = "_ZdaPv";
  }
} cxxbuiltin_table;

bool cxxbuiltin(const symbol* entry, std::string* res)
{
  std::string name = entry->m_name;
  std::map<std::string, std::string>::const_iterator p =
    cxxbuiltin_table.find(name);
  if ( p != cxxbuiltin_table.end() ){
    *res = p->second;
    return true;
  }
  else
    return false;
}

std::string scope_name(symbol_tree* scope)
{
  if ( tag* T = dynamic_cast<tag*>(scope) )
    return scope_name(scope->m_parent) + func_name(T->name());
  if ( _namespace* nmsp = dynamic_cast<_namespace*>(scope) )
    return nmsp->name();
  return "";
}

class conv_fname {
  std::string* m_res;
public:
  conv_fname(std::string* res) : m_res(res) {}
  void operator()(int c)
  {
    switch ( c ){
    case ' ':   *m_res += "sp"; break;
    case '~':   *m_res += "ti"; break;
    case '[':   *m_res += "lb"; break;
    case ']':   *m_res += "rb"; break;
    case '(':   *m_res += "lp"; break;
    case ')':   *m_res += "rp"; break;
    case '&':   *m_res += "an"; break;
    case '<':   *m_res += "lt"; break;
    case '>':   *m_res += "gt"; break;
    case '=':   *m_res += "eq"; break;
    case '!':   *m_res += "ne"; break;
    case '+':   *m_res += "pl"; break;
    case '-':   *m_res += "mi"; break;
    case '*':   *m_res += "mu"; break;
    case '/':   *m_res += "di"; break;
    case '%':   *m_res += "mo"; break;
    case '^':   *m_res += "xo"; break;
    case '|':   *m_res += "or"; break;
    case ',':   *m_res += "cm"; break;
    case '\'':  *m_res += "da"; break;
    default:    *m_res += char(c); break;
    }
  }
};

std::string func_name(std::string name)
{
  std::string res;
  std::for_each(name.begin(),name.end(),conv_fname(&res));
  return res;
}

void entry_func_sub(const symbol* func)
{
  if ( func->m_templ_func )
    return;
  if ( address_descriptor.find(func) == address_descriptor.end() ){
    std::string label;
    if ( !cxxbuiltin(func,&label) ){
      label = scope_name(func->m_scope);
      label += func_name(func->m_name);
      if ( !func->m_csymbol )
        label += signature(func->m_type);
    }
    address_descriptor[func] = new mem(label,func->m_type);
  }
}

void entry_func(const std::pair<std::string,std::vector<symbol*> >& pair)
{
  const std::vector<symbol*>& vec = pair.second;
  std::for_each(vec.begin(),vec.end(),entry_func_sub);
}
#endif // CXX_GENERATOR
