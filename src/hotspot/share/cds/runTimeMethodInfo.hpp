#include "classfile/compactHashtable.hpp"
#include "memory/metaspaceClosure.hpp"
#include "oops/method.hpp"
#include "oops/methodCounters.hpp"
#include "oops/methodData.hpp"

class RunTimeMethodDictionary;

class MethodInfo: public CHeapObj<mtClass>{
private:
  Method* _method;
  MethodCounters* _mc;
  MethodData* _md;

public:
  MethodInfo(Method* method) {
    _method = method;
    _mc = method->method_counters();
    _md = method->method_data();
  }
  void init(MethodInfo* dumptime_info);

  Method* method() const { return _method; }
  void set_method(Method* method) { _method = method; }
  MethodCounters* method_counters() const { return _mc; }
  void set_method_counters(MethodCounters* mc) { _mc = mc; }
  MethodData* method_data() const { return _md; }
  void set_method_data(MethodData* md) { _md = md; }

  void metaspace_pointers_do(MetaspaceClosure* it) {
    it->push(&_method);
    it->push(&_mc);
    it->push(&_md);
  }

  static size_t byte_size() {
    return align_up(sizeof(MethodInfo), wordSize);
  }

  static inline bool EQUALS(const MethodInfo* value, Method* key, int len_unused) {
    return (value->method() == key);
  }
};

class MethodInfoTable: AllStatic { 
private: 
  static GrowableArray<MethodInfo*> _method_info_list;
  static RunTimeMethodDictionary _method_dictionary;
 
public: 
  static void initialize();
  static void add(Method* md);
  static size_t estimate_size_for_archive();
  static void make_shareable();
  static void metaspace_pointers_do(MetaspaceClosure* it);
  static void dump();
  static void serialize_shared_table_header(SerializeClosure* soc);
  static bool find_method_info(Method* method);
};

class RunTimeMethodDictionary: public OffsetCompactHashtable< 
  Method*, 
  const MethodInfo*, 
  MethodInfo::EQUALS> {};

