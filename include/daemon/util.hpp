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
/**
 * @brief Utility functions for the daemon context.
 */

#ifndef GEKKOFS_DAEMON_UTIL_HPP
#define GEKKOFS_DAEMON_UTIL_HPP

namespace gkfs::utils {
/**
 * @brief Registers the daemon's RPC address to the shared hosts file.
 * // lxl 将守护进程的rpc地址注册到host_file
 * @throws std::runtime_error when file stream fails
 */
void
populate_hosts_file();

/**
 * @brief Attempts to remove the entire hosts file.
 * // lxl 删除整个host_file
 */
void
destroy_hosts_file();
} // namespace gkfs::utils

#endif // GEKKOFS_DAEMON_UTIL_HPP
