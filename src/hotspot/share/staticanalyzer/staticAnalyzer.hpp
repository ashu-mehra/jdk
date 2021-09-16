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

#ifndef SHARE_STATICANALYZER_STATICANALYZER_HPP
#define SHARE_STATICANALYZER_STATICANALYZER_HPP

#include "ci/ciMethod.hpp"
#include "compiler/abstractCompiler.hpp"

class StaticAnalyzer : public AbstractCompiler {
 private:
  //static bool init_analyzer_runtime();

public:
  StaticAnalyzer() : AbstractCompiler(static_analyzer) {}

  // Name
  const char *name() { return "SA"; }
  void initialize();

  // Compilation entry point for methods
  void compile_method(ciEnv* env,
                    ciMethod* target,
                    int entry_bci,
                    bool install_code,
                    DirectiveSet* directive);
};

#endif // SHARE_STATICANALYZER_STATICANALYZER_HPP