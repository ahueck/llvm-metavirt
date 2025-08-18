// metavirt library
// Copyright (c) 2025 metavirt authors
// Distributed under the BSD 3-Clause License license.
// (See accompanying file LICENSE)
// SPDX-License-Identifier: BSD-3-Clause

#include "metavirt/DITypeExtractor.h"

#include "metavirt/DIFinder.h"
#include "metavirt/DIRootType.h"
#include "metavirt/ValuePath.h"
#include "support/Logger.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/Instructions.h"

#include <utility>

namespace metavirt::type {

/// Strips a potentially-derived debug information type until the base type is reached.
///
/// @param ty Type to strip
///
/// @return Base type of @p ty
[[nodiscard]] static auto strip_ty(const llvm::DIType* ty) {
  while (ty && isa<llvm::DIDerivedType>(ty))
    ty = dyn_cast<llvm::DIDerivedType>(ty)->getBaseType();

  return ty;
}

/// Determines if a given call corresponds to a `dynamic_cast`.
///
/// @param call Call base
///
/// @return `true` if @p call is a `dynamic_cast`, `false` otherwise
static bool is_dyn_cast(const llvm::CallBase* call) {
  return call->getCalledFunction() && call->getCalledFunction()->getName().contains("__dynamic_cast");
}

/// Locates a `dynamic_cast` in a value path.
///
/// @param path Value path
///
/// @return Pointer to the `dynamic_cast` invocation or `nullptr` if none was found
static const llvm::CallBase* locate_dyn_cast(const dataflow::ValuePath& path) {
  for (const auto* inst : path.path_to_value) {
    if (const auto* call = dyn_cast_or_null<llvm::CallBase>(inst); call && is_dyn_cast(call))
      return dyn_cast<llvm::CallBase>(inst);
  }

  return {};
}

/// Tries to determine whether the given value path contains an upcast.
///
/// @param path Value path
///
/// @return The offset of the upcast or `std::nullopt` if no upcast could be detected
static std::optional<int64_t> extract_upcast(const dataflow::ValuePath& path) {
  // TODO(laurin): Rewrite using patch matchers once those work well enough
  for (const auto [i, inst] : enumerate(path.path_to_value)) {
    if (const auto* cast = dyn_cast_or_null<llvm::CallBase>(inst); !cast || !is_dyn_cast(cast))
      continue;

    if (!isa<llvm::GetElementPtrInst>(*path.at(i - 1)))
      continue;

    const auto* gep = dyn_cast<llvm::GetElementPtrInst>(*path.at(i - 1));
    assert(gep);

    if (const auto* load = dyn_cast_or_null<llvm::LoadInst>(gep->getOperand(1));
        load && isa<llvm::GetElementPtrInst>(load->getPointerOperand())) {
      const auto* inner_gep = dyn_cast<llvm::GetElementPtrInst>(load->getPointerOperand());

      // We're dealing with an upcast so make sure the GEP index is negative
      if (const auto off = dyn_cast<llvm::ConstantInt>(inner_gep->getOperand(1)); off && off->getValue().isNegative()) {
        return off->getSExtValue();
      }
    }
  }

  return {};
}

/// Determines whether there's an inheritance tag present in the debug information that matches
/// offset information extracted during upcast matching.
///
/// @param base_ty Base class determined through root value analysis
/// @param cast_target_ty Target type of the `dynamic_cast`
/// @param off Derived class offset extracted from upcast matching
/// @param dbg_finder Populated debug info finder
///
/// @return `true` if a matching tag was found, `false` otherwise
[[nodiscard]] static bool matching_inheritance_tag(const llvm::DIType* base_ty, const llvm::DIType* cast_target_ty,
                                                   const int64_t off, const llvm::DebugInfoFinder& dbg_finder) {
  for (const auto* ty : dbg_finder.types()) {
    if (!isa<llvm::DIDerivedType>(ty) || ty->getTag() != llvm::dwarf::DW_TAG_inheritance)
      continue;

    const auto* derived_ty = dyn_cast<llvm::DIDerivedType>(ty);
    assert(derived_ty);

    // Make sure to use the absolute of `off` as the GEP index will be negative
    if (derived_ty->getBaseType() == base_ty && derived_ty->getScope() == cast_target_ty &&
        derived_ty->getOffsetInBits() == std::abs(off)) {
      return true;
    }
  }

  return false;
}

/// Tries to compute the target type of a `dynamic_cast`.
///
/// @param path Value path
/// @param base_ty Base class determined through root value analysis prior
/// @param dbg_finder Populated debug info finder
///
/// @return Target type of the `dynamic_cast` or `std::nullopt` if none was found
static std::optional<std::pair<const llvm::DIType*, const llvm::DIType*>> resolve_dyn_cast_target(
    const dataflow::ValuePath& path, const llvm::DIType* base_ty, const llvm::DebugInfoFinder& dbg_finder) {
  const auto* cast = locate_dyn_cast(path);
  if (!cast)
    return {};

  // Get the target type of the dynamic cast from an attached debug value annotation
  const auto var = difinder::find_local_variable(cast);
  if (!var)
    return {};

  const auto* cast_target_ty = strip_ty((*var)->getType());
  assert(cast_target_ty);

  // If we can detect an upcast and have a matching inheritance tag, return the base class too
  if (const auto off = extract_upcast(path); off && matching_inheritance_tag(base_ty, cast_target_ty, *off, dbg_finder))
    return {{base_ty, cast_target_ty}};

  return {{nullptr, cast_target_ty}};
}

/// Returns the set of all (transitively) derived classes of class @p base_ty.
///
/// @param base_ty Base class
/// @param dbg_finder Populated debug info finder
///
/// @return Set of all derived classes of class @p base_ty
static llvm::SmallVector<const llvm::DIType*> derived_tys_for_base(const llvm::DIType* base_ty,
                                                                   const llvm::DebugInfoFinder& dbg_finder) {
  llvm::SmallVector<const llvm::DIType*> r{};

  for (const auto* ty : dbg_finder.types()) {
    if (const auto* class_ty = dyn_cast_or_null<llvm::DICompositeType>(ty); class_ty) {
      for (const auto* cur_ty = class_ty; cur_ty && cur_ty->getElements().size() >= 1;) {
        const auto* inherit = dyn_cast_or_null<llvm::DIDerivedType>(cur_ty->getElements()[0]);

        if (!inherit || inherit->getTag() != llvm::dwarf::DW_TAG_inheritance)
          break;

        if (inherit->getBaseType() == base_ty) {
          r.push_back(class_ty);
          break;
        }

        cur_ty = dyn_cast_or_null<llvm::DICompositeType>(inherit->getBaseType());
      }
    }
  }

  return r;
}

/// Attempts to resolve the root type of the given value path.
///
/// @param call_path (Call base, value path) pair leading to a root type
/// @param dbg_finder Populated debug information finder
///
/// @return Set of potential types or `std::nullopt` if none could be found
std::optional<PotentialTys> find_types(const dataflow::CallValuePath& call_path,
                                       const llvm::DebugInfoFinder& dbg_finder) {
  const auto type = root::find_type_root(call_path);
  if (!type) {
    LOG_DEBUG("Failed to compute root type for [" << call_path.path << "]");
    return {};
  }

  // Resolve the base type by stripping derived types
  const auto* const base_ty = strip_ty(*type);

  // Walk the value path once more to check for `dynamic_cast`s, in which case we only want to return
  // the target type of the cast as it restricts the available methods.
  // In cases where the call is `A::base_method()` with `base_method()` not being overridden in `A`, the
  // compiler will automatically generate an upcast to `Base` before the call so `base_ty` here will be
  // non-null, in addition to returning the target type of the dynamic cast.
  // Otherwise, `base_ty` will be `nullptr` and only the derived type is returned.
  if (const auto cast_target = resolve_dyn_cast_target(call_path.path, base_ty, dbg_finder); cast_target) {
    const auto [base_ty, derived_ty] = *cast_target;
    return {{base_ty, {derived_ty}}};
  }

  // Return the base class as well as all derived classes. The set of derived types can later be constrained based
  // on the vtable index and subprogram debug information to rule out classes that are be impossible.
  return {{base_ty, derived_tys_for_base(base_ty, dbg_finder)}};
}

}  // namespace metavirt::type
