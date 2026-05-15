//
// span.h
// by Naomi Peori <naomi@peori.ca>
//

#pragma once
#include <cstddef>

template <typename T>
struct Span {
  const T *data = nullptr;
  size_t size   = 0;
  constexpr bool empty() const noexcept { return size == 0; }
};
