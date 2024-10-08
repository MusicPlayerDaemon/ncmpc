// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include <utility>

#if defined(_LIBCPP_VERSION) && defined(__clang__) && __clang_major__ < 14
/* libc++ until 14 has the coroutine definitions in the
   std::experimental namespace */

#include <experimental/coroutine>

namespace std {
using std::experimental::coroutine_handle;
using std::experimental::suspend_never;
using std::experimental::suspend_always;
using std::experimental::noop_coroutine;
}

#else /* not clang */

#include <coroutine>
#ifndef __cpp_impl_coroutine
#error Need -fcoroutines
#endif

#endif /* not clang */
