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
#include "llvm/ADT/SmallPtrSet.h"
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
  for (const auto& item : enumerate(path.path_to_value)) {
    // Note: we need to use this instead of structured bindings to be compatible with Clang Versions < 16
    auto i    = item.index();
    auto inst = item.value();
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
  llvm::SmallVector<const llvm::DIType*> derived_classes{};

  auto resolve_through_typedefs = [](const llvm::DIType* type) -> const llvm::DICompositeType* {
    while (type) {
      if (const auto* composite = dyn_cast<llvm::DICompositeType>(type)) {
        return composite;
      }
      const auto* derived = dyn_cast<llvm::DIDerivedType>(type);
      if (!derived || derived->getTag() != llvm::dwarf::DW_TAG_typedef) {
        break;
      }
      type = derived->getBaseType();
    }
    return nullptr;
  };

  // Returns true if start_node inherits from target_base:
  const auto has_inheritance_relationship = [&](const llvm::DICompositeType* start_node) -> bool {
    llvm::SmallVector<const llvm::DICompositeType*, 8> worklist;
    llvm::SmallPtrSet<const llvm::DICompositeType*, 8> visited;

    worklist.push_back(start_node);
    visited.insert(start_node);

    while (!worklist.empty()) {
      const auto* curr = worklist.pop_back_val();

      for (const auto* element : curr->getElements()) {
        const auto* inheritance = dyn_cast_or_null<llvm::DIDerivedType>(element);

        if (!inheritance || inheritance->getTag() != llvm::dwarf::DW_TAG_inheritance) {
          continue;
        }

        // Could call this instead, but keep it to typedefs for now: strip_ty(inheritance->getBaseType());
        const auto* base_part = resolve_through_typedefs(inheritance->getBaseType());

        // Found the target base?
        if (base_part == base_ty) {
          return true;
        }
        // Otherwise, add the parent to the worklist to continue searching up.
        if (const auto* next = dyn_cast_or_null<llvm::DICompositeType>(base_part)) {
          if (visited.insert(next).second) {
            worklist.push_back(next);
          }
        }
      }
    }
    return false;
  };

  for (const auto* ty : dbg_finder.types()) {
    const auto* class_ty = dyn_cast_or_null<llvm::DICompositeType>(ty);
    if (!class_ty || class_ty == base_ty) {
      continue;
    }

    if (has_inheritance_relationship(class_ty)) {
      derived_classes.push_back(class_ty);
    }
  }

  return derived_classes;
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
