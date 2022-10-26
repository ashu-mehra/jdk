/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates. All rights reserved.
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

#include "precompiled.hpp"
#include "cds/archiveHeapLoader.inline.hpp"
#include "cds/filemap.hpp"
#include "cds/heapShared.hpp"
#include "cds/metaspaceShared.hpp"
#include "classfile/classLoaderDataShared.hpp"
#include "classfile/systemDictionaryShared.hpp"
#include "gc/shared/collectedHeap.hpp"
#include "logging/log.hpp"
#include "memory/iterator.inline.hpp"
#include "memory/resourceArea.hpp"
#include "memory/universe.hpp"
#include "utilities/bitMap.inline.hpp"
#include "utilities/copy.hpp"

#if INCLUDE_CDS_JAVA_HEAP

void ArchiveHeapLoader::fixup_regions() {
  FileMapInfo* mapinfo = FileMapInfo::current_info();
  if (can_use()) {
    mapinfo->fill_failed_mapped_regions();
  }
  if (FileMapInfo::is_archived_heap_available()) {
    if (!MetaspaceShared::use_full_module_graph()) {
      // Need to remove all the archived java.lang.Module objects from HeapShared::roots().
      ClassLoaderDataShared::clear_archived_oops();
    }
  }
  SystemDictionaryShared::update_archived_mirror_native_pointers();
}

#endif // INCLUDE_CDS_JAVA_HEAP
