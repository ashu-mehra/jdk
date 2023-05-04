#include "cds/archiveBuilder.hpp"
#include "cds/runTimeMethodInfo.hpp"
#include "classfile/classLoaderDataGraph.hpp"
#include "classfile/compactHashtable.hpp"
#include "classfile/systemDictionaryShared.hpp"
#include "logging/logMessage.hpp"
#include "memory/iterator.hpp"
#include "memory/resourceArea.hpp"
#include "oops/method.hpp"
#include "oops/methodCounters.hpp"
#include "oops/methodData.hpp"
#include "runtime/handles.inline.hpp"

GrowableArray<MethodInfo*> MethodInfoTable::_method_info_list(4*1024, mtClassShared);
RunTimeMethodDictionary MethodInfoTable::_method_dictionary;

void collect_methods(Method* method) {
  if (method->method_data() != nullptr) {
    MethodInfoTable::add(method);
  }
}

void filter_shared_class(Klass* klass) {
  if (klass->is_instance_klass() &&
      (klass->is_shared() || !ArchiveBuilder::current()->is_excluded(klass))) {
    InstanceKlass* iklass = (InstanceKlass*)klass;
    iklass->methods_do(collect_methods);
  }
}

void MethodInfoTable::initialize() {
  ClassLoaderDataGraph::classes_do(filter_shared_class);
}

void MethodInfoTable::add(Method* method) {
  _method_info_list.append(new MethodInfo(method));
}

size_t MethodInfoTable::estimate_size_for_archive() {
  size_t size = MethodInfo::byte_size() * _method_info_list.length();
  size += CompactHashtableWriter::estimate_size(_method_info_list.length());
  return size;
}

void MethodInfoTable::make_shareable() {
  for (int i = 0; i < _method_info_list.length(); i++) {
    MethodInfo* info = _method_info_list.at(i);
    MethodCounters* mc = info->method_counters();
    if (mc != nullptr) {
      mc->remove_unshareable_info();
    }
    MethodData* md = info->method_data();
    md->remove_unshareable_info();
  }
}

void MethodInfoTable::metaspace_pointers_do(MetaspaceClosure* it) {
  for (int i = 0; i < _method_info_list.length(); i++) {
    MethodInfo* info = _method_info_list.at(i);
    info->metaspace_pointers_do(it);
  }
}

void MethodInfo::init(MethodInfo* dumptime_info) {
  ArchiveBuilder* builder = ArchiveBuilder::current();
  Method* method = dumptime_info->method();
  MethodCounters* mc = dumptime_info->method_counters();
  MethodData* md = dumptime_info->method_data();
  assert(mc == nullptr || builder->is_in_buffer_space(mc), "must be");
  assert(md == nullptr || builder->is_in_buffer_space(md), "must be");

  _method = method;
  _mc = mc;
  _md = md;

  ArchivePtrMarker::mark_pointer(&_method);
  ArchivePtrMarker::mark_pointer(&_mc);
  ArchivePtrMarker::mark_pointer(&_md);
}

class MethodInfoTableCopier {
private:
  CompactHashtableWriter* _writer;
  ArchiveBuilder* _builder;
public:
  MethodInfoTableCopier(CompactHashtableWriter* writer)
    : _writer(writer), _builder(ArchiveBuilder::current()) {}

  void process(MethodInfo* dumptime_info) {
    MethodInfo* record = (MethodInfo*)ArchiveBuilder::ro_region_alloc(MethodInfo::byte_size());
    record->init(dumptime_info);

    unsigned int hash = SystemDictionaryShared::hash_for_shared_dictionary((address)dumptime_info->method());
    u4 delta = _builder->buffer_to_offset_u4((address)record);
    LogMessage(cds, hashtables) msg;
    if (msg.is_info()) {
      ResourceMark rm;
      Method* method = dumptime_info->method();
      msg.info("MethodInfoTable entry for %s::%s%s : (%u, %u)",
               method->method_holder()->external_name(),
               method->name()->as_C_string(),
               method->signature()->as_C_string(),
               hash, delta);
    }
    _writer->add(hash, delta);
  }
};

void MethodInfoTable::dump() {
  CompactHashtableStats stats;
  _method_dictionary.reset();
  CompactHashtableWriter writer(_method_info_list.length(), &stats);
  MethodInfoTableCopier copier(&writer);
  for (int i = 0; i < _method_info_list.length(); i++) {
    copier.process(_method_info_list.at(i));
  }
  writer.dump(&_method_dictionary, "method data table");
}

void MethodInfoTable::serialize_shared_table_header(SerializeClosure* soc) {
  _method_dictionary.serialize_header(soc);
}

bool MethodInfoTable::find_method_info(Method* method) {
  unsigned int hash = SystemDictionaryShared::hash_for_shared_dictionary_quick(method);
  const MethodInfo* info = _method_dictionary.lookup(method, hash, 0 /* unused */);
  if (info != nullptr) {
    assert(info->method() == method, "sanity check");
    methodHandle mh(Thread::current(), method);
    MethodCounters* mc = info->method_counters();
    if (mc != nullptr) {
      mc->restore_unshareable_info(mh);
      mh->init_method_counters(mc);
    }

    MethodData* md = info->method_data();
    if (md != nullptr) {
      md->restore_unshareable_info();
      mh->init_method_data(md);
    }
    LogMessage(cds, hashtables) msg;
    if (msg.is_info()) {
      ResourceMark rm;
      msg.info("MethodInfoTable::find_method_info for %s::%s%s with hash=%u returned %p",
               method->method_holder()->external_name(),
               method->name()->as_C_string(),
               method->signature()->as_C_string(),
               hash, info);
    }
    return true;
  }
  return false;
}

