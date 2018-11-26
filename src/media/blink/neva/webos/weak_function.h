// Copyright (c) 2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MEDIA_BLINK_NEVA_WEBOS_WEAK_FUNCTION_H_
#define MEDIA_BLINK_NEVA_WEBOS_WEAK_FUNCTION_H_

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"

namespace media {
namespace neva {
// WeakFunction can be used as a function that can invoke callback in the given
// task only when given weakptr is valid.
// The task_runner argument should be the same task where the weakptr was
// created.
template <typename Functor, typename T>
class WeakFunction {
 public:
  WeakFunction(const base::Location& posted_from,
               scoped_refptr<base::SingleThreadTaskRunner> task_runner,
               Functor&& callback,
               base::WeakPtr<T>&& ptr)
      : posted_from_(posted_from),
        task_runner_(std::move(task_runner)),
        callback_(callback),
        ptr_(ptr) {}
  template <class... CallArgs>
  void operator()(CallArgs&&... args) {
    task_runner_->PostTask(
        posted_from_,
        base::Bind(callback_, ptr_, std::forward<CallArgs>(args)...));
  }

 private:
  base::Location posted_from_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  Functor callback_;
  base::WeakPtr<T> ptr_;
};

// Helper template method to create WeakFunction
template <typename Functor, typename T>
WeakFunction<Functor, T> BindToLoop(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    Functor callback,
    base::WeakPtr<T> ptr) {
  return WeakFunction<Functor, T>(FROM_HERE, task_runner,
                                  std::forward<Functor>(callback),
                                  std::forward<base::WeakPtr<T>>(ptr));
}

}  // namespace neva
}  // namespace media

#endif  // MEDIA_BLINK_NEVA_WEBOS_WEAK_FUNCTION_H_
