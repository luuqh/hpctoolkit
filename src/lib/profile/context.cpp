// -*-Mode: C++;-*-

// * BeginRiceCopyright *****************************************************
//
// $HeadURL$
// $Id$
//
// --------------------------------------------------------------------------
// Part of HPCToolkit (hpctoolkit.org)
//
// Information about sources of support for research and development of
// HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
// --------------------------------------------------------------------------
//
// Copyright ((c)) 2019-2020, Rice University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage.
//
// ******************************************************* EndRiceCopyright *

#include "context.hpp"

#include "module.hpp"
#include "metric.hpp"

#include <stack>
#include <stdexcept>
#include <vector>

using namespace hpctoolkit;

Context::Context(ud_t::struct_t& rs, Context* p, Scope&& s)
  : userdata(rs, std::ref(*this)), children_p(new children_t()),
    u_parent(p), u_scope(s) {};
Context::Context(Context&& c)
  : userdata(std::move(c.userdata), std::ref(*this)),
    children_p(new children_t()),
    u_parent(c.direct_parent()), u_scope(c.scope()) {};

Context::~Context() noexcept {
  // C++ generates a recursive algorithm for this by default
  // So we replace it with a post-order tree traversal
  try {
    struct frame_t {
      frame_t(Context& c)
        : ctx(c), iter(c.children_p->iterate()),
          here(iter.begin()), end(iter.end()) {};
      Context& ctx;
      using iter_t = decltype(children_p->iterate());
      iter_t iter;
      decltype(iter.begin()) here;
      decltype(iter.end()) end;
    };
    std::stack<frame_t, std::vector<frame_t>> stack;
    if(children_p) stack.emplace(*this);
    while(!stack.empty()) {
      if(stack.top().here != stack.top().end) {
        // We have more children to handle
        Context& c = *stack.top().here++;
        if(c.children_p) stack.emplace(c);
        continue;
      }

      Context& c = stack.top().ctx;
      stack.pop();
      c.children_p.reset(nullptr);
    }
  } catch(...) {};  // If we run into errors, just let the recursion handle it.
}

std::pair<Context&,bool> Context::ensure(Scope&& s) {
  auto x = children_p->emplace(userdata.base(), this, std::move(s));
  return {x.first(), x.second};
}
