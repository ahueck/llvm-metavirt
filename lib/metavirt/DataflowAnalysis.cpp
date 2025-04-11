#include "metavirt/DefUseAnalysis.h"
#include "metavirt/ValuePath.h"
#include "support/Logger.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Operator.h"

#include <llvm/IR/InstrTypes.h>

namespace metavirt::dataflow {
struct AnchorMatcher {
  llvm::SmallVector<ValuePath, 4> anchors;

  auto operator()(const ValuePath& path) -> decltype(DefUseChain::kContinue) {
    using namespace llvm;

    const auto current_value = path.value();
    if (!current_value) {
      return DefUseChain::kSkip;
    }

    const auto* inst = dyn_cast<Instruction>(current_value.value());
    if (!inst) {
      return DefUseChain::kSkip;
    }

    if (llvm::isa<IntrinsicInst>(inst)) {
      return DefUseChain::kSkip;
    }

    switch (inst->getOpcode()) {
      case Instruction::Ret: {
        anchors.push_back(path);
        return DefUseChain::kCancel;
      }
      case Instruction::Store: {
        const auto* store = cast<StoreInst>(inst);
        if (store->getPointerOperand() != path.start_value()) {
          anchors.push_back(path);
        } else {
          LOG_DEBUG("Store to allocated object, skipping. " << *store)
        }

        // kSkip instead of kCancel, as multiple stores can be present for a "malloc-like" call, see test
        // "heap_lulesh_domain_mock.cpp" at higher optim:
        return DefUseChain::kSkip;
      }
      case Instruction::GetElementPtr: {
        // Currently, we do not care about "new -> gep -> ...",
        // since this may be C++ init of the allocated object
        return DefUseChain::kSkip;
      }
      case Instruction::Call:  // NOLINT
        [[fallthrough]];
      case Instruction::CallBr:
        [[fallthrough]];
      case Instruction::Invoke: {
        if (path.only_start()) {
          return DefUseChain::kContinue;
        }

        // TODO Maybe check if we have a constructor call here, and not take it if it is?
        anchors.push_back(path);

        return DefUseChain::kContinue;
      }
    }
    return DefUseChain::kContinue;
  }
};

struct TargetMatcher {
  llvm::SmallVector<ValuePath, 4> types_path;

  auto operator()(const ValuePath& path) -> decltype(DefUseChain::kContinue) {
    // This builds the path (backtrack) from store to LHS target (argument, alloca etc.)
    using namespace llvm;

    const auto current_value = path.value();

    if (!current_value) {
      LOG_DEBUG("Current value is null, skipping")
      return DefUseChain::kSkip;
    }

    const auto* value = current_value.value();

    // Handle path to alloca -> can extract type
    if (const auto* inst = dyn_cast<Instruction>(value)) {
      if (isa<IntrinsicInst>(inst)) {
        return DefUseChain::kSkip;
      }
      if (isa<AllocaInst>(inst)) {
        types_path.emplace_back(path);
        return DefUseChain::kSkip;  // maybe kCancel is better?
      }
    }

    if (llvm::isa<llvm::Argument>(value)) {
      types_path.emplace_back(path);
      return DefUseChain::kSkip;
    }

    if (llvm::isa<llvm::GlobalVariable>(value)) {
      types_path.emplace_back(path);
      return DefUseChain::kSkip;
    }

    if (llvm::isa<llvm::CallBase>(value)) {
      const auto* call = dyn_cast<CallInst>(value);
      if (const auto f = call->getCalledFunction(); !f || !f->getName().contains("__dynamic_cast"))
        types_path.emplace_back(path);

      return DefUseChain::kContinue;
    }

    return DefUseChain::kContinue;
  }
};

struct BacktrackSearch {
  using ValueRange = llvm::TinyPtrVector<const llvm::Value*>;

  auto operator()(const ValuePath& path) -> std::optional<ValueRange> {
    // Backtracks from target (a store) to, e.g., argument/global/etc.
    using namespace llvm;

    ValueRange result;

    if (auto const_expr = llvm::dyn_cast_or_null<llvm::ConstantExpr>(path.value().value_or(nullptr))) {
      if (const_expr->getOpcode() == Instruction::GetElementPtr) {
        if (auto op = llvm::dyn_cast<llvm::GEPOperator>(const_expr)) {
          result.push_back(op->getPointerOperand());
        }
        return result;
      }
      if (const_expr->getOpcode() == Instruction::BitCast) {
        LOG_DEBUG("Found constant bitcast from value" << *const_expr->getOperand(0))
        result.push_back(const_expr->getOperand(0));
        return result;
      }
      LOG_ERROR("Unsupported ConstantExpr for path generation " << *const_expr)
    }

    const auto* inst = dyn_cast_or_null<Instruction>(path.value().value_or(nullptr));
    if (inst == nullptr) {
      return {};
    }

    LOG_DEBUG("Backtracking from " << *inst);

    switch (inst->getOpcode()) {
      case Instruction::Store: {
        result.push_back(llvm::dyn_cast<StoreInst>(inst)->getPointerOperand());
        return result;
      }
      case Instruction::Load: {
        result.push_back(llvm::dyn_cast<LoadInst>(inst)->getPointerOperand());
        return result;
      }
      case Instruction::GetElementPtr: {
        result.push_back(llvm::dyn_cast<GetElementPtrInst>(inst)->getPointerOperand());
        return result;
      }
      case Instruction::Call: {
        const auto call = dyn_cast<CallBase>(inst);
        assert(call);

        // Push the pointer argument to `dynamic_cast`s
        if (const auto f = call->getCalledFunction(); f && f->getName().contains("__dynamic_cast")) {
          result.push_back(call->getArgOperand(0));
          return result;
        }

        result.push_back(call->getCalledOperand());
        return result;
      }
      case Instruction::BitCast: {
        result.push_back(llvm::dyn_cast<llvm::BitCastInst>(inst)->getOperand(0));
        return result;
      }
      case Instruction::PHI: {
        auto* phi = llvm::dyn_cast<PHINode>(inst);
        for (auto& incoming : phi->incoming_values()) {
          LOG_DEBUG("  > Backtracking phi incoming " << *incoming.get());
          result.push_back(incoming.get());
        }
        return result;
      }
      default:
        return {};
    }
  }
};

llvm::SmallVector<ValuePath, 4> type_for_virt_call(const llvm::CallBase* call) {
  using namespace llvm;

  auto should_search = [&](const ValuePath&) -> bool { return true; };
  DefUseChain value_traversal;

  // backward: find paths from anchor (call) to alloca/argument/global etc.
  TargetMatcher malloc_anchor_backtrack;
  BacktrackSearch backtrack_search_dir_fn;
  SmallVector<ValuePath, 4> ditype_paths;

  value_traversal.traverse_custom(call, malloc_anchor_backtrack, should_search, backtrack_search_dir_fn);
  for (const auto& backtrack_path : malloc_anchor_backtrack.types_path) {
    LOG_DEBUG("Found backtrack path " << backtrack_path)
    ditype_paths.emplace_back(backtrack_path);
  }

  LOG_DEBUG("\n")
  LOG_DEBUG("Final paths to types:");
  for (const auto& path : ditype_paths) {
    LOG_DEBUG("  T: " << path);
  }

  return ditype_paths;
}

llvm::SmallVector<ValuePath, 4> path_from_alloca(const llvm::AllocaInst* alloca) {
  using namespace llvm;

  auto should_search = [&](const ValuePath&) -> bool { return true; };
  DefUseChain value_traversal;

  AnchorMatcher malloc_forward_anchor_finder;
  value_traversal.traverse(alloca, malloc_forward_anchor_finder, should_search);
  return malloc_forward_anchor_finder.anchors;
}

}  // namespace metavirt::dataflow