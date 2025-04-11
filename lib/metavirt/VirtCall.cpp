// metavirt library
// Copyright (c) 2025 metavirt authors
// Distributed under the BSD 3-Clause License license.
// (See accompanying file LICENSE)
// SPDX-License-Identifier: BSD-3-Clause

#include "VirtCall.h"

#include "metavirt/DITypeExtractor.h"
#include "metavirt/DataflowAnalysis.h"
#include "metavirt/PathMatchers.h"
#include "metavirt/Util.h"
#include "metavirt/ValuePath.h"
#include "support/Logger.h"

#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instructions.h"

namespace metavirt {
/// Maps a set of value paths to their corresponding root types.
///
/// @param call Call base for the virtual call
/// @param paths Value paths leading to root types
/// @param dbg_finder Populated debug info finder
///
/// @return Set of root types found at the end of @p paths
static llvm::SmallVector<type::PotentialTys> collect_types(const llvm::CallBase* call,
                                                           const llvm::ArrayRef<dataflow::ValuePath> paths,
                                                           const llvm::DebugInfoFinder& dbg_finder) {
  llvm::SmallVector<type::PotentialTys> types;

  transform(paths, util::optional_back_inserter(types),
            [&](const auto& path) { return type::find_types(dataflow::CallValuePath{call, path}, dbg_finder); });

  return types;
}

/// Attempts to resolve the vtable index of the call encoded in a value path.
///
/// N.B. that in the presence of virtual destructors, the resolved index might not correspond directly to the
///      lexical index of the virtual function in-source, due to the presence of an additional deleting
///      destructor in the virtual table.
///
/// @param paths Value paths for the call base collected by the dataflow analysis
///
/// @return Virtual table index of the called function or `std::nullopt` if it couldn't be resolved
static std::optional<size_t> try_resolve_vtable_index(const llvm::ArrayRef<dataflow::ValuePath> paths) {
  static auto is_byte_index = [](const llvm::GetElementPtrInst* gep) -> bool {
    return gep->getSourceElementType()->isIntegerTy(8);
  };

  std::optional<size_t> idx{};

  // Examine the value paths for the following pattern in order to resolve the vtable index of our call base:
  //
  //                                                        ┌────────────────────────────────────┐
  //                                                  ┌────►│%p = getelementptr i8, ptr %x, i64 N│
  //                                                  │     └────────────────────────────────────┘
  //                                                  │     If we're looking at a GEP, try to extract
  // ┌──────────────────────┐    ┌────────────────────┴┐    the index if it's a constant.
  // │call T %f(ptr %c, ...)├───►│%f = load ptr, ptr %p│
  // └──────────────────────┘    └────────────────────┬┘
  // Initial call base of the                         │
  // virtual call.                                    │     ┌─────────────────────┐
  //                                                  └────►│%p = load ptr, ptr %x│
  //                                                        └─────────────────────┘
  //                                                        If we're looking at another load, the
  //                                                        vtable index must be 0.
  // clang-format off
  auto pat
    = seq(
        load(),
        any_of(
          gep_with_constant([&idx](const llvm::GetElementPtrInst* gep, const llvm::ConstantInt* c) {
            idx.emplace(is_byte_index(gep) ? c->getZExtValue() / 8 : c->getZExtValue());
          }),
          load([&idx](const llvm::LoadInst*) { idx.emplace(0); })
        )
      );
  // clang-format on

  // Run the pattern on all paths
  for (const auto& path : paths)
    static_cast<void>(pat.run_match(path));

  return idx;
}

/// Computes the set of potential call targets associated with the virtual call.
///
/// @param potential_tys Set of potential class types
/// @param vtable_index Virtual index of the called function if available
/// @param dbg_finder Debug finder instance with populated type information
///
/// @return Map of classes to potential functions that might be invoked by the virtual call
static auto call_target_closure(const std::unordered_set<const llvm::DIType*>& potential_tys,
                                const std::optional<size_t> vtable_index, const llvm::DebugInfoFinder& dbg_finder) {
  std::unordered_map<const llvm::DIType*, llvm::SmallVector<const llvm::DISubprogram*>> call_targets{};

  for (const auto* f : dbg_finder.subprograms()) {
    // Filter for virtual functions that are defined by one of our potential class types
    if (!is_contained(potential_tys, f->getContainingType()) || !f->getVirtuality())
      continue;

    // If we have an index available, filter for it
    if (!vtable_index.has_value() || (vtable_index.has_value() && f->getVirtualIndex() == *vtable_index))
      call_targets[f->getContainingType()].push_back(f);
  }

  return call_targets;
}

/// Flattens structured potential type information into a set of types.
///
/// @param potential_tys List of potential type information
///
/// @return Set of all types contained in @p potential_tys
static auto flatten_potential_tys(llvm::ArrayRef<type::PotentialTys> const& potential_tys) {
  std::unordered_set<const llvm::DIType*> r{};

  for (const auto& [base_ty, derived_tys] : potential_tys) {
    // Base class could be non-existent
    if (base_ty)
      r.insert(base_ty);

    for (const auto* derived_ty : derived_tys)
      r.insert(derived_ty);
  }

  return r;
}

/// Attempts to recover virtual call information for the given call base.
///
/// @param call Virtual call base
///
/// @return Virtual call data or `std::nullopt` if none could be recovered
std::optional<VcallData> vcall_data_for(const llvm::CallBase* call) {
  llvm::DebugInfoFinder dbg_finder{};
  dbg_finder.processModule(*call->getModule());

  // Compute value paths for the given call
  const auto paths = dataflow::type_for_virt_call(call);

  // Generate the set of potential class types associated with the call
  const auto tys = flatten_potential_tys(collect_types(call, paths, dbg_finder));
  if (tys.empty())
    return {};

  // Try to compute the virtual table index of the called function
  const auto vtable_index = try_resolve_vtable_index(paths);

  return VcallData{
      .types        = tys,
      .call_targets = call_target_closure(tys, vtable_index, dbg_finder),
      .vtable_index = vtable_index,
  };
}

/// Convenience helper to return a list of (name, origin) pairs for the call targets
/// recorded in `VcallData`.
///
/// @param data Virtual call information
///
/// @return List of (name, origin) pairs for all call targets in @p data
llvm::SmallVector<NamesAndOrigin> fn_names_and_origins(const VcallData& data) {
  llvm::SmallVector<NamesAndOrigin> r{};

  for (const auto& [ty, fns] : data.call_targets) {
    for (const auto* fn : fns)
      r.push_back({.name = fn->getLinkageName(), .origin = fn->getFilename()});
  }

  return r;
}

}  // namespace metavirt
