/*
  Copyright 2018-2022, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2022, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  This file is part of GekkoFS.

  GekkoFS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  GekkoFS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with GekkoFS.  If not, see <https://www.gnu.org/licenses/>.

  SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef GEKKOFS_PATH_UTIL_HPP
#define GEKKOFS_PATH_UTIL_HPP

#include <string>
#include <vector>

namespace gkfs::path {

constexpr unsigned int max_length = 4096; // 4k chars

constexpr char separator = '/'; // PATH SEPARATOR

bool
is_relative(const std::string& path); // 是否为相对路径

bool
is_absolute(const std::string& path); // 是否为绝对路径

bool
has_trailing_slash(const std::string& path);// 是否有末尾斜杠

std::string
prepend_path(const std::string& path, const char* raw_path); // 前缀路径组合

std::string
absolute_to_relative(const std::string& root_path,
                     const std::string& absolute_path); // unused ATM 绝对转相对

std::string
dirname(const std::string& path); //返回目录名的完全路径

std::vector<std::string>
split_path(const std::string& path); // 分隔路径

} // namespace gkfs::path

#endif // GEKKOFS_PATH_UTIL_HPP
