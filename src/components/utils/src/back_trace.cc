/*
 * Copyright (c) 2013, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "utils/back_trace.h"

#include <algorithm>
#include <vector>
#include <sstream>

#ifdef OS_WIN32
#else
#include <cxxabi.h>
#include <execinfo.h>
#endif

#include "utils/macro.h"

using std::ostream;
using std::string;
using std::vector;
using threads::Thread;

namespace utils {

namespace {
string demangle(const char* symbol) {
#ifdef OS_WIN32
	return "";
#else
  char temp[2048];
  if (1 == sscanf(symbol, "%*[^(]%*[^_]%2047[^)+]", temp)) {
    size_t size;
    int32_t status;
    char* demangled = abi::__cxa_demangle(temp, NULL, &size, &status);
    if (demangled != NULL) {
      string result(demangled);
      free(demangled);
      return result;
    }
  }
  return symbol;
#endif
}
}

Backtrace::Backtrace(int32_t count, int32_t skip_top)
    : thread_id_(threads::Thread::CurrentId()) {
#ifdef OS_WIN32
#else
  int32_t skip = skip_top + 1; // Skip this constructor
  vector<void*> full_trace (count + skip);
  int32_t captured = backtrace(&full_trace.front(), count + skip);
  int32_t first_call = std::min(captured, skip);
  int32_t last_call = std::min(first_call + count, captured);
  backtrace_.assign(full_trace.begin() + first_call, full_trace.begin() + last_call);
#endif
}

Backtrace::~Backtrace() {
}

vector<string> Backtrace::CallStack() const {
  vector<string> callstack;
#ifdef OS_WIN32
#else
  callstack.reserve(backtrace_.size());
  char** mangled = backtrace_symbols(&backtrace_.front(), backtrace_.size());
  for (size_t i = 0; i != backtrace_.size(); ++i) {
    callstack.push_back(demangle(mangled[i]));
  }
  free(mangled);
#endif
  return callstack;
}

threads::PlatformThreadHandle Backtrace::ThreadId() const {
  return thread_id_;
}

ostream& operator<< (ostream& os, const Backtrace& bt) {
  const vector<string> symbols = bt.CallStack();
#if defined(OS_WIN32) || defined(OS_WINCE)
  os<<"Stack trace ("<<bt.ThreadId().p<<")\n";
#else
  os<<"Stack trace ("<<bt.ThreadId()<<")\n";
#endif
  if (symbols.empty()) {
    os<<"Not available"<<std::endl;
  } else for (size_t i = 0; i < symbols.size(); ++i) {
    os<<symbols[i]<<std::endl;
  }
  return os;
}

} // namespace utils
