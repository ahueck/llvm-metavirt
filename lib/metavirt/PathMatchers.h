#ifndef PATH_MATCHERS_HPP
#define PATH_MATCHERS_HPP

#include <functional>
#include <metavirt/ValuePath.h>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace metavirt {

namespace detail {

template <typename T>
struct first_arg;

template <typename R, typename Arg, typename... Args>
struct first_arg<R (*)(Arg, Args...)> {
  using type = Arg;
};

template <typename C, typename R, typename Arg, typename... Args>
struct first_arg<R (C::*)(Arg, Args...)> {
  using type = Arg;
};

template <typename C, typename R, typename Arg, typename... Args>
struct first_arg<R (C::*)(Arg, Args...) const> {
  using type = Arg;
};

template <typename C, typename R, typename Arg, typename... Args>
struct first_arg<R (C::*)(Arg, Args...) noexcept> {
  using type = Arg;
};

template <typename C, typename R, typename Arg, typename... Args>
struct first_arg<R (C::*)(Arg, Args...) const noexcept> {
  using type = Arg;
};

template <typename T>
struct first_arg {
  using type = typename first_arg<decltype(&T::operator())>::type;
};

template <typename T>
using first_arg_t = typename first_arg<std::decay_t<T>>::type;

}  // namespace detail

template <typename R>
struct ErasedFn final {
  template <typename F>
  ErasedFn(F&& f)
    requires requires {
      typename detail::first_arg_t<F>;

      requires !std::is_same_v<std::decay_t<F>, ErasedFn>;
      requires std::is_pointer_v<detail::first_arg_t<F>>;
      // Ensure we receive a callable that is cast-able
      requires std::is_base_of_v<llvm::Instruction, std::remove_pointer_t<std::remove_cv_t<detail::first_arg_t<F>>>>;
    }
  {
    using target_ty = std::remove_pointer_t<detail::first_arg_t<F>>;

    this->f = [f = std::forward<F>(f)](const llvm::Instruction* inst) -> R {
      // Cast back to the concrete derived type the callable expects
      return f(dyn_cast_or_null<target_ty>(inst));
    };
  }

  [[nodiscard]] R operator()(const llvm::Instruction* inst) noexcept {
    return this->f(inst);
  }

 private:
  std::function<R(const llvm::Instruction*)> f;
};

// Convenience aliases for erased callable types used by the pattern logic
using ErasedAction = ErasedFn<void>;
using ErasedPred   = ErasedFn<bool>;
using ErasedProj   = ErasedFn<const llvm::Instruction*>;

/// Instruction pattern to match against a single instruction.
struct Inst {
  /// Creates a new instruction pattern.
  ///
  /// @param op Instruction opcode
  /// @param pred Predicate to match against the instruction
  /// @param then Action to run on match
  /// @param proj Optional projection
  explicit Inst(
      const unsigned op, ErasedPred&& pred = [](const llvm::Instruction*) { return true; },
      ErasedAction&& then = [](const llvm::Instruction*) {}, std::optional<ErasedProj>&& proj = {})
      : op{op}, pred{std::move(pred)}, then{std::move(then)}, proj{std::move(proj)} {
  }

  /// Matches the pattern.
  ///
  /// @param inst Instruction to match against
  ///
  /// @return `true` if the instruction's opcode matches the pattern's opcode and the pattern's predicate
  ///         returns `true`, `false` otherwise
  [[nodiscard]] bool operator()(const llvm::Instruction* inst) noexcept {
    if (inst->getOpcode() != op)
      return false;

    return pred(inst);
  }

  /// Executes the pattern.
  ///
  /// @param inst Instruction to match against
  void run(const llvm::Instruction* inst) noexcept {
    return then(inst);
  }

 private:
  unsigned op;
  ErasedPred pred;
  ErasedAction then;
  std::optional<ErasedProj> proj;
};

/// Creates an instruction pattern that matches a load instruction.
///
/// @param then Action to run on match
/// @param proj Optional projection
///
/// @return Instruction pattern matching a load instruction
inline auto load(ErasedAction&& then = [](const llvm::LoadInst*) {}, std::optional<ErasedProj>&& proj = {}) {
  return Inst{llvm::Instruction::Load, [](const llvm::LoadInst*) { return true; }, std::move(then), std::move(proj)};
}

/// Creates an instruction pattern that matches a GEP instruction.
///
/// @param pred Predicate to match against a given GEP instruction
/// @param then Action to run on match
/// @param proj Optional projection
///
/// @return Instruction pattern matching a GEP instruction
inline auto gep(
    ErasedPred&& pred   = [](const llvm::GetElementPtrInst*) { return true; },
    ErasedAction&& then = [](const llvm::GetElementPtrInst*) {}, std::optional<ErasedProj>&& proj = {}) {
  return Inst{llvm::Instruction::GetElementPtr, std::move(pred), std::move(then), std::move(proj)};
}

/// Creates an instruction pattern that matches a GEP instruction with an immediate index.
///
/// @param then Action to run on match
/// @param proj Optional projection
///
/// @return Instruction pattern that matches a GEP instruction with an immediate index
inline auto gep_with_constant(
    std::function<void(const llvm::GetElementPtrInst*, const llvm::ConstantInt*)>&& then =
        [](const llvm::GetElementPtrInst*, const llvm::ConstantInt*) {},
    std::optional<ErasedProj>&& proj = {}) {
  return Inst{llvm::Instruction::GetElementPtr,
              [](const llvm::GetElementPtrInst* gep) {
                if (const auto* c = dyn_cast<llvm::ConstantInt>(gep->getOperand(1)); c)
                  return true;

                return false;
              },
              [then = std::move(then)](const llvm::GetElementPtrInst* gep) {
                then(gep, dyn_cast<llvm::ConstantInt>(gep->getOperand(1)));
              },
              std::move(proj)};
}

/// Conjunction of a set of patterns.
///
/// @tparam Ps Types of wrapped patterns
template <typename... Ps>
struct AllOf {
  /// Creates a conjunction of a set of patterns.
  ///
  /// @tparam Pats Deduced pattern types for perfect forwarding
  /// @param pats List of patterns to wrap
  template <typename... Pats>
    requires(std::is_same_v<std::remove_cvref_t<Pats>, Ps> && ...)
  explicit constexpr AllOf(Pats&&... pats) noexcept((std::is_nothrow_constructible_v<std::remove_cvref_t<Pats>, Ps> &&
                                                     ...))
      : pats{std::forward<Ps>(pats)...} {
  }

  /// Matches the pattern.
  ///
  /// @param inst Instruction to match against
  ///
  /// @return `true` if all wrapped patterns match @p inst, `false` otherwise
  [[nodiscard]] bool operator()(const llvm::Instruction* inst) noexcept {
    return [this, inst]<size_t... Is>(std::index_sequence<Is...>) {
      return (std::get<Is>(pats)(inst) && ...);
    }(std::make_index_sequence<sizeof...(Ps)>{});
  }

  /// Executes all wrapped patterns.
  ///
  /// @param inst Instruction to match against
  void run(const llvm::Instruction* inst) noexcept {
    // We can just run all the patterns because they all have to match in order to get here
    return [this, inst]<size_t... Is>(std::index_sequence<Is...>) {
      (std::get<Is>(pats).run(inst), ...);
    }(std::make_index_sequence<sizeof...(Ps)>{});
  }

 private:
  std::tuple<std::remove_reference_t<Ps>...> pats;
};

template <typename... Pats>
explicit AllOf(Pats&&... pats) -> AllOf<std::remove_cvref_t<Pats>...>;

/// Creates a conjunction of a set of patterns.
///
/// @tparam Pats Types of wrapped patterns
/// @param pats List of patterns to wrap
///
/// @return Conjunction of @p pats
template <typename... Pats>
constexpr auto all_of(Pats&&... pats) noexcept(
    std::is_nothrow_constructible_v<AllOf<std::remove_cvref_t<Pats>...>, Pats...>) {
  return AllOf{std::forward<Pats>(pats)...};
}

/// Disjunction of a set of patterns.
///
/// @tparam Ps Types of wrapped patterns
template <typename... Ps>
struct AnyOf {
  /// Creates a new disjunction of a list of patterns.
  ///
  /// @tparam Pats Deduced types of wrapped patterns for perfect forwarding
  /// @param pats List of patterns to wrap
  template <typename... Pats>
    requires(std::is_same_v<std::remove_cvref_t<Pats>, Ps> && ...)
  explicit constexpr AnyOf(Pats&&... pats) noexcept((std::is_nothrow_constructible_v<std::remove_cvref_t<Pats>, Ps> &&
                                                     ...))
      : pats{std::forward<Ps>(pats)...} {
  }

  /// Matches the pattern.
  ///
  /// @param inst Instruction to match against
  ///
  /// @return `true` if any of the wrapped patterns match @p inst, `false` otherwise
  [[nodiscard]] bool operator()(const llvm::Instruction* inst) noexcept {
    return [this, inst]<size_t... Is>(std::index_sequence<Is...>) {
      return (std::get<Is>(pats)(inst) || ...);
    }(std::make_index_sequence<sizeof...(Ps)>{});
  }

  /// Executes those wrapped patterns that match @p inst.
  ///
  /// @param inst Instruction to match against
  void run(const llvm::Instruction* inst) noexcept {
    return [this, inst]<size_t... Is>(std::index_sequence<Is...>) {
      const auto run = [this, inst]<size_t I>(auto&& pat) noexcept {
        // Only run the patterns that actually match
        if (pat(inst))
          pat.run(inst);
      };

      (run.template operator()<Is>(std::get<Is>(pats)), ...);
    }(std::make_index_sequence<sizeof...(Ps)>{});
  }

 private:
  std::tuple<std::remove_cvref_t<Ps>...> pats;
};

template <typename... Pats>
explicit AnyOf(Pats&&... pats) -> AnyOf<std::remove_cvref_t<Pats>...>;

/// Creates a disjunction of a list of patterns.
///
/// @tparam Pats Types of wrapped patterns
/// @param pats List of patterns to wrap
///
/// @return Disjunction of @p pats
template <typename... Pats>
constexpr auto any_of(Pats&&... pats) noexcept(
    std::is_nothrow_constructible_v<AnyOf<std::remove_cvref_t<Pats>...>, Pats...>) {
  return AnyOf{std::forward<Pats>(pats)...};
}

/// Ordered sequence of patterns.
///
/// @tparam Ps Wrapped pattern types
template <typename... Ps>
struct Seq {
  /// Constructs a sequence pattern from a list of patterns.
  ///
  /// @tparam Pats Deduced pattern types for perfect forwarding
  /// @param pats List of patterns to wrap
  template <typename... Pats>
    requires(std::is_same_v<std::remove_cvref_t<Pats>, Ps> && ...)
  explicit constexpr Seq(Pats&&... pats) noexcept((std::is_nothrow_constructible_v<std::remove_cvref_t<Pats>, Ps> &&
                                                   ...))
      : pats{std::forward<Ps>(pats)...} {
  }

  /// Matches all wrapped patterns against the given value path.
  /// N.B. Patterns have to match adjacent instructions in the path for the whole sequence to match.
  ///
  /// @param path Value path to match against
  ///
  /// @return `true` if all wrapped patterns matched, `false` otherwise
  [[nodiscard]] bool run_match(const dataflow::ValuePath& path) noexcept {
    for (size_t i = 0; i < path.size(); ++i) {
      const auto matches = [this, path, i]<size_t... Is>(std::index_sequence<Is...>) {
        const auto run = []<size_t I>(auto&& pat, std::optional<const llvm::Value*>&& inst) {
          if (inst.has_value() && isa<llvm::Instruction>(*inst) && pat(dyn_cast<llvm::Instruction>(*inst))) {
            pat.run(dyn_cast<llvm::Instruction>(*inst));
            return true;
          }

          return false;
        };

        return (run.template operator()<Is>(std::get<Is>(pats), path.at(i + Is)) && ...);
      }(std::make_index_sequence<sizeof...(Ps)>{});

      if (matches)
        return true;
    }

    return false;
  }

 private:
  std::tuple<std::remove_cvref_t<Ps>...> pats;
};

template <typename... Pats>
explicit Seq(Pats&&... pats) -> Seq<std::remove_cvref_t<Pats>...>;

/// Creates a new sequence pattern from a list of patterns.
///
/// @tparam Pats Types of wrapped patterns
/// @param pats List of patterns to wrap
///
/// @return Sequence pattern wrapping @p pats
template <typename... Pats>
constexpr auto seq(Pats&&... pats) noexcept(
    std::is_nothrow_constructible_v<Seq<std::remove_cvref_t<Pats>...>, Pats...>) {
  return Seq{std::forward<Pats>(pats)...};
}

}  // namespace metavirt

#endif
