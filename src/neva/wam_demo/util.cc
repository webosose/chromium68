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

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <unistd.h>

#include <util.h>

namespace util {

const char* get_my_ip_to(const char* ip, int port) {
  if (!ip)
    return "unknown";

  struct sockaddr_in sa;
  sa.sin_family  = AF_INET;
  sa.sin_port = htons(port);

  if (inet_aton(ip, &sa.sin_addr) == 0)
    return "invalid";

  int s = socket(AF_INET, SOCK_DGRAM, 0);
  if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
    close(s);
    return "unreachable";
  }

  struct sockaddr_in me;
  socklen_t mel = sizeof(me);
  if (getsockname(s, (struct sockaddr *)&me, &mel) == 0)
    if (me.sin_family == AF_INET) {
      close(s);
      return inet_ntoa(me.sin_addr);
    }

  close(s);
  return "error";
}

} // namespace util
