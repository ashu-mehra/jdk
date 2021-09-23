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

#include "ci/bcReachabilityAnalyzer.hpp"
#include "compiler/compilerDirectives.hpp"
#include "memory/oopFactory.hpp"
#include "runtime/handles.inline.hpp"
#include "runtime/javaCalls.hpp"
#include "staticanalyzer/saJavaClasses.hpp"
#include "staticanalyzer/staticAnalyzer.hpp"

void StaticAnalyzer::initialize() {
  JavaThread* THREAD = JavaThread::current();
  jdk_internal_staticanalysis_StaticAnalyzer::compute_offsets(THREAD); // should this be moved to saJavaClasses.cpp
}

void StaticAnalyzer::compile_method(ciEnv* env, ciMethod* target, int entry_bci, bool install_code, DirectiveSet* directive) {
  JavaThread* THREAD = JavaThread::current();
  // do reachability analysis
  BCReachabilityAnalyzer *bcra = target->get_bcra();

  // TODO: load classes discovered by bcra

  //make upcall to add callees in the set of methods to be analyzed
  GrowableArray<ciMethod*> callees = bcra->get_callees();
  int numCallees = callees.length();
  if (numCallees > 0) {
    typeArrayOop calleeHandles = oopFactory::new_longArray(numCallees, CHECK);
    int i = 0;
    for (GrowableArrayIterator<ciMethod*> it = callees.begin(); it != callees.end(); ++it) {
      ciMethod *method = *it;
      jlong methodId = (jlong)method->get_Method();
      calleeHandles->long_at_put(i, methodId);
      i += 1;
    }
    JavaValue result(T_OBJECT);
    JavaCallArguments args;
    args.push_oop(Handle(THREAD, calleeHandles));
    JavaCalls::call_static(&result, jdk_internal_staticanalysis_StaticAnalyzer::klass(), vmSymbols::addToQueue_name(), vmSymbols::addToQueue_name_signature(), &args, THREAD);
  }
}

