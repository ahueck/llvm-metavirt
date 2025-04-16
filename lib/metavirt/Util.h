//  metavirt library
//  Copyright (c) 2022-2025 metavirt authors
//  Distributed under the BSD 3-Clause license.
//  (See accompanying file LICENSE)
//  SPDX-License-Identifier: BSD-3-Clause
//

#ifndef METAVIRT_UTIL_H
#define METAVIRT_UTIL_H

#include <functional>
#include <optional>

namespace metavirt::util {

template <typename F>
class ScopeExit {
 public:
  template <typename Fwd>
  explicit ScopeExit(Fwd&& exit_fn) : exit_fn{std::forward<Fwd>(exit_fn)} {
  }

  ScopeExit(const ScopeExit&)            = delete;
  ScopeExit& operator=(const ScopeExit&) = delete;

  ~ScopeExit() {
    std::invoke(exit_fn);
  }

 private:
  F exit_fn;
};

template <typename F>
explicit ScopeExit(F&&) -> ScopeExit<std::remove_reference_t<F>>;

template <typename F>
ScopeExit<F> create_scope_exit(F&& exit_fn) {
  return ScopeExit{std::forward<F>(exit_fn)};
}

namespace detail {
template <typename Container>
struct OptionalBackInsertIterator {
  using iterator_category = std::output_iterator_tag;
  using value_type        = typename Container::value_type;
  using difference_type   = void;
  using pointer           = void;
  using reference         = void;

  explicit OptionalBackInsertIterator(Container& container) : container_(std::addressof(container)) {
  }

  OptionalBackInsertIterator<Container>& operator=(const std::optional<value_type> opt) {
    if (opt) {
      container_->emplace_back(std::move(opt.value()));
    }
    return *this;
  }

  OptionalBackInsertIterator<Container>& operator*() {
    return *this;
  }

  OptionalBackInsertIterator<Container>& operator++() {
    return *this;
  }

  OptionalBackInsertIterator<Container>& operator++(int) {
    return *this;
  }

 protected:
  Container* container_;
};

}  // namespace detail

template <typename Container>
detail::OptionalBackInsertIterator<Container> optional_back_inserter(Container& container) {
  return detail::OptionalBackInsertIterator<Container>(container);
}

}  // namespace metavirt::util

#endif  // METAVIRT_UTIL_H
