//  metavirt library
//  Copyright (c) 2022-2025 metavirt authors
//  Distributed under the BSD 3-Clause license.
//  (See accompanying file LICENSE)
//  SPDX-License-Identifier: BSD-3-Clause
//

#ifndef METAVIRT_DATAFLOWANALYSIS_H
#define METAVIRT_DATAFLOWANALYSIS_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Instructions.h"

namespace metavirt::dataflow {
struct ValuePath;

llvm::SmallVector<ValuePath, 4> type_for_virt_call(const llvm::CallBase* call);
llvm::SmallVector<ValuePath, 4> path_from_alloca(const llvm::AllocaInst* alloca);
}  // namespace metavirt::dataflow

#endif  // METAVIRT_DATAFLOWANALYSIS_H
