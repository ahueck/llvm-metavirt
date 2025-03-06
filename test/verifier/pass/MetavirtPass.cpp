// metavirt library
// Copyright (c) 2025 metavirt authors
// Distributed under the BSD 3-Clause License license.
// (See accompanying file LICENSE)
// SPDX-License-Identifier: BSD-3-Clause

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <cstdlib>
#include <metavirt/VirtCall.h>
#include <support/Logger.h>

using namespace llvm;

namespace metavirt {

class MetavirtPass : public llvm::PassInfoMixin<MetavirtPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module&, llvm::ModuleAnalysisManager&);

  bool runOnModule(llvm::Module&);

  bool runOnFunc(llvm::Function&);
};

class LegacyMetavirtPass : public llvm::ModulePass {
 private:
  MetavirtPass pass_impl_;

 public:
  static char ID;  // NOLINT

  LegacyMetavirtPass() : ModulePass(ID) {};

  bool runOnModule(llvm::Module& module) override;

  ~LegacyMetavirtPass() override = default;
};

llvm::PreservedAnalyses MetavirtPass::run(llvm::Module& module, llvm::ModuleAnalysisManager&) {
  const auto changed  = runOnModule(module);
  auto dump_module_if = [](const llvm::Module& module, std::string_view env_var,
                           llvm::raw_ostream& out_s = llvm::outs()) {
    const auto* env_val = std::getenv(env_var.data());
    if (env_val) {
      module.print(out_s, nullptr);
    }
  };
  dump_module_if(module, "METAVIRT_DUMP_IR");
  return changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
}

bool MetavirtPass::runOnModule(llvm::Module& module) {
  const auto changed = llvm::count_if(module.functions(), [&](auto& func) { return runOnFunc(func); }) > 1;
  return changed;
}

bool LegacyMetavirtPass::runOnModule(llvm::Module& module) {
  const auto modified = pass_impl_.runOnModule(module);
  return modified;
}

bool MetavirtPass::runOnFunc(llvm::Function& function) {
  if (function.isDeclaration()) {
    return false;
  }

  for (auto& inst :
       make_filter_range(instructions(function), [](auto const& inst) { return isa<llvm::CallBase>(inst); })) {
    auto const data = virtcall::virtual_type_for(dyn_cast<CallBase>(&inst));
    if (!data.has_value())
      continue;

    LOG_DEBUG("Got virtcall data");
  }

  return false;
}

}  // namespace metavirt

#define DEBUG_TYPE "metavirt-pass"

//.....................
// New PM
//.....................
llvm::PassPluginLibraryInfo getMetavirtPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "metavirt", LLVM_VERSION_STRING, [](PassBuilder& pass_builder) {
            pass_builder.registerPipelineStartEPCallback(
                [](auto& module_pm, OptimizationLevel) { module_pm.addPass(metavirt::MetavirtPass()); });
            pass_builder.registerPipelineParsingCallback(
                [](StringRef name, ModulePassManager& module_pm, ArrayRef<PassBuilder::PipelineElement>) {
                  if (name == "metavirt") {
                    module_pm.addPass(metavirt::MetavirtPass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return getMetavirtPassPluginInfo();
}

//.....................
// Old PM
//.....................
char metavirt::LegacyMetavirtPass::ID = 0;  // NOLINT

static RegisterPass<metavirt::LegacyMetavirtPass> x("metavirt",
                                                    "Metavirt Pass");  // NOLINT

ModulePass* createMetavirtPass() {
  return new metavirt::LegacyMetavirtPass();
}

extern "C" void AddMetavirtPass(LLVMPassManagerRef pass_manager) {
  unwrap(pass_manager)->add(createMetavirtPass());
}
