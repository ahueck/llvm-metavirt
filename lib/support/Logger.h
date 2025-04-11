// metavirt library
// Copyright (c) 2025 metavirt authors
// Distributed under the BSD 3-Clause License license.
// (See accompanying file LICENSE)
// SPDX-License-Identifier: BSD-3-Clause

#ifndef VIRTCALL_LOGGER_H
#define VIRTCALL_LOGGER_H

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <string_view>

#ifndef VIRTCALL_LOG_LEVEL
/*
 * Usually set at compile time: -DVIRTCALL_LOG_LEVEL=<N>, N in [0, 4] for output
 * 4 being most verbose with [Trace]
 */
#define VIRTCALL_LOG_LEVEL 3
#endif

#ifndef VIRTCALL_LOG_BASENAME
#define VIRTCALL_LOG_BASENAME __FILE__
#endif

#ifndef VIRTCALL_MPI_LOGGER
#define VIRTCALL_MPI_LOGGER 0
#endif

namespace metavirt::log {
class LogContext {
 private:
  llvm::Module* context{nullptr};
  LogContext(llvm::Module* mod = nullptr) : context{mod} {
  }

 public:
  void setModule(llvm::Module* mod) {
    context = mod;
  }

  llvm::Module* getModule() {
    return context;
  }

  static LogContext& get() {
    static LogContext context;
    return context;
  }
};

inline std::string ditype_str(const llvm::Metadata* type) {
  if (type == nullptr) {
    return "";
  }
  auto* module = LogContext::get().getModule();
  std::string logging_message;
  llvm::raw_string_ostream rso(logging_message);
  type->print(rso, module);
  return rso.str();
}

inline void virtcall_log(const std::string_view msg) {
  llvm::dbgs() << msg;
}
}  // namespace metavirt::log

#define VIRTCALL_LOG_LEVEL_MSG(LEVEL_NUM, LEVEL, MSG)                                                                 \
  if ((LEVEL_NUM) <= VIRTCALL_LOG_LEVEL) {                                                                            \
    std::string logging_message;                                                                                    \
    llvm::raw_string_ostream rso(logging_message);                                                                  \
    rso << (LEVEL) << VIRTCALL_LOG_BASENAME << ":" << __func__ << ":" << __LINE__ << ":" << MSG << "\n"; /* NOLINT */ \
    metavirt::log::virtcall_log(rso.str());                                                                             \
  }

#define VIRTCALL_LOG_LEVEL_MSG_BARE(LEVEL_NUM, LEVEL, MSG) \
  if ((LEVEL_NUM) <= VIRTCALL_LOG_LEVEL) {                 \
    std::string logging_message;                         \
    llvm::raw_string_ostream rso(logging_message);       \
    rso << (LEVEL) << " " << MSG << "\n"; /* NOLINT */   \
    metavirt::log::virtcall_log(rso.str());                  \
  }

#define LOG_TRACE(MSG) VIRTCALL_LOG_LEVEL_MSG_BARE(4, "[Trace]", MSG)
#define LOG_DEBUG(MSG) VIRTCALL_LOG_LEVEL_MSG(3, "[Debug]", MSG)
#define LOG_INFO(MSG) VIRTCALL_LOG_LEVEL_MSG(2, "[Info]", MSG)
#define LOG_WARNING(MSG) VIRTCALL_LOG_LEVEL_MSG(1, "[Warning]", MSG)
#define LOG_ERROR(MSG) VIRTCALL_LOG_LEVEL_MSG(1, "[Error]", MSG)
#define LOG_FATAL(MSG) VIRTCALL_LOG_LEVEL_MSG(0, "[Fatal]", MSG)
#define LOG_MSG(MSG) VIRTCALL_LOG_LEVEL_MSG_BARE(0, "", MSG)

#endif  // VIRTCALL_LOGGER_H
