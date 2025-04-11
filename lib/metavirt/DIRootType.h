//  metavirt library
//  Copyright (c) 2022-2025 metavirt authors
//  Distributed under the BSD 3-Clause license.
//  (See accompanying file LICENSE)
//  SPDX-License-Identifier: BSD-3-Clause
//

#ifndef METAVIRT_DIROOTTYPE_H
#define METAVIRT_DIROOTTYPE_H

#include <optional>

namespace metavirt::dataflow {
struct CallValuePath;
}  // namespace metavirt::dataflow

namespace llvm {
class DIType;
class CallBase;
}  // namespace llvm

namespace metavirt::root {

std::optional<llvm::DIType*> find_type_root(const dataflow::CallValuePath& path);

}

#endif  // METAVIRT_DIROOTTYPE_H
