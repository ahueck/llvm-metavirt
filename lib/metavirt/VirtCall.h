// metavirt library
// Copyright (c) 2025 metavirt authors
// Distributed under the BSD 3-Clause License license.
// (See accompanying file LICENSE)
// SPDX-License-Identifier: BSD-3-Clause

#ifndef VIRTCALL_VIRTCALL_H
#define VIRTCALL_VIRTCALL_H

#include <optional>

namespace llvm {
class CallBase;
}  // namespace llvm

namespace virtcall {

struct VirtcallData {
};

std::optional<VirtcallData> virtual_type_for(const llvm::CallBase*);

}

#endif
