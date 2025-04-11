// metavirt library
// Copyright (c) 2025 metavirt authors
// Distributed under the BSD 3-Clause License license.
// (See accompanying file LICENSE)
// SPDX-License-Identifier: BSD-3-Clause

#ifndef METAVIRT_VIRTCALL_H
#define METAVIRT_VIRTCALL_H

#include "llvm/IR/DebugInfoMetadata.h"

#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace llvm {

class CallBase;

}

namespace metavirt {

/// Recovered information for a virtual call
struct VcallData {
  /// Set of class types whose virtual functions are potential call targets
  std::unordered_set<const llvm::DIType*> types;
  /// Map from [class] -> [list of call targets] for all potential call targets
  std::unordered_map<const llvm::DIType*, llvm::SmallVector<const llvm::DISubprogram*>> call_targets;
  /// Virtual function table index of the call
  std::optional<size_t> vtable_index;
};

std::optional<VcallData> vcall_data_for(const llvm::CallBase*);

/// String-based function information
struct NamesAndOrigin {
  /// Mangled function name
  llvm::StringRef name;
  /// File name of the containing translation unit
  llvm::StringRef origin;
};

llvm::SmallVector<NamesAndOrigin> fn_names_and_origins(const VcallData&);

}  // namespace metavirt

#endif
