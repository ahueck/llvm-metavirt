//  metavirt library
//  Copyright (c) 2022-2025 metavirt authors
//  Distributed under the BSD 3-Clause license.
//  (See accompanying file LICENSE)
//  SPDX-License-Identifier: BSD-3-Clause
//

#ifndef METAVIRT_DITYPEEXTRACTOR_H
#define METAVIRT_DITYPEEXTRACTOR_H

#include "metavirt/ValuePath.h"

#include <llvm/IR/DebugInfo.h>
#include <optional>

namespace metavirt::type {

struct PotentialTys {
  // Base class or `nullptr` if a concrete derived type was resolved
  const llvm::DIType* base_ty;
  // Set of potential derived classes that could be associated with the call
  llvm::SmallVector<const llvm::DIType*> derived_tys;
};

std::optional<PotentialTys> find_types(const dataflow::CallValuePath&, const llvm::DebugInfoFinder&);

}  // namespace metavirt::type

#endif  // METAVIRT_DITYPEEXTRACTOR_H
