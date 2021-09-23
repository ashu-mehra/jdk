/*
 * Copyright (c) 1999, 2021, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "classfile/systemDictionary.hpp"
#include "classfile/vmSymbols.hpp"
#include "compiler/compilationPolicy.hpp"
#include "jni.h"
#include "memory/resourceArea.hpp"
#include "memory/universe.hpp"
#include "oops/instanceKlass.hpp"
#include "oops/klass.hpp"
#include "runtime/interfaceSupport.inline.hpp"
#include "runtime/jniHandles.inline.hpp"
#include "staticanalyzer/saJavaClasses.hpp"
#include "utilities/globalDefinitions.hpp"

InstanceKlass* jdk_internal_staticanalysis_StaticAnalyzer::_klass = NULL;

void jdk_internal_staticanalysis_StaticAnalyzer::compute_offsets(TRAPS) {
  Klass* k = SystemDictionary::resolve_or_fail(vmSymbols::jdk_internal_staticanalysis_StaticAnalyzer(), true, CHECK);
  _klass = InstanceKlass::cast(k);
  _klass->initialize(CHECK);
}

JVM_ENTRY(jboolean, SA_analyze(JNIEnv *env, jclass saClass, jlongArray handles))
  if (handles != NULL) {
    typeArrayOop handles_oop = (typeArrayOop) JNIHandles::resolve(handles);
    for (int i = 0; i < handles_oop->length(); i++) {
      Method *method = (Method *)handles_oop->long_at(i);
      methodHandle mh(thread, method);
      CompilationPolicy::analyze(mh, CHECK_(JNI_FALSE));
    }
  }
  return JNI_TRUE;
JVM_END

// copied from JVM_RegisterJVMCINatives
JVM_ENTRY(void, JVM_RegisterStaticAnalyzerNatives(JNIEnv *env, jclass saClass))
{
  ResourceMark rm(thread);
  HandleMark hm(thread);
  ThreadToNativeFromVM trans(thread);

  // Ensure _non_oop_bits is initialized
  Universe::non_oop_word();

  if (JNI_OK != env->RegisterNatives(saClass, jdk_internal_staticanalysis_StaticAnalyzer::methods, jdk_internal_staticanalysis_StaticAnalyzer::methods_count())) {
    if (!env->ExceptionCheck()) {
      for (int i = 0; i < jdk_internal_staticanalysis_StaticAnalyzer::methods_count(); i++) {
        JNINativeMethod *jniNativeMethod = jdk_internal_staticanalysis_StaticAnalyzer::methods + i;
        if (JNI_OK != env->RegisterNatives(saClass, jniNativeMethod, 1)) {
          guarantee(false, "Error registering JNI method %s%s", jniNativeMethod->name, jniNativeMethod->signature);
          break;
        }
      }
    } else {
      env->ExceptionDescribe();
    }
    guarantee(false, "Failed registering CompilerToVM native methods");
  }
}
JVM_END

#define CC (char*)  /*cast a literal from (const char*)*/
#define FN_PTR(f) CAST_FROM_FN_PTR(void*, &(SA_ ## f))

JNINativeMethod jdk_internal_staticanalysis_StaticAnalyzer::methods[] = {
  {CC "analyze", CC "([J)V", FN_PTR(analyze)},
};

int jdk_internal_staticanalysis_StaticAnalyzer::methods_count() {
  return sizeof(methods) / sizeof(JNINativeMethod);
}