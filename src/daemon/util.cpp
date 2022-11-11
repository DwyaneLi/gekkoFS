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

#include <daemon/util.hpp>
#include <daemon/daemon.hpp>

#include <common/rpc/rpc_util.hpp>

using namespace std;

namespace gkfs::utils {

/**
 * @internal
 * Appends a single line to an existing shared hosts file with the RPC
 * connection information of this daemon. If it doesn't exist, it is created.
 * The line includes the hostname (and rootdir_suffix if applicable) and the RPC
 * server's listening address.
 * 使用此守护进程的RPC连接信息将一行附加到现有的共享主机文件。如果它不存在，它就会被创建。
 * 这一行包含主机名(如果适用，还有rootdir_suffix)和RPC服务器的监听地址。
 * NOTE, the shared file system must support strong consistency semantics to
 * ensure each daemon can write its information to the file even if the write
 * access is simultaneous.
 * 共享文件系统必须支持强一致性语义，以确保每个守护进程可以将其信息写入文件，即使写访问是同时进行的。
 * 
 * 这里好像是写入本地文件，也就是说这个是共享的本地文件？
 * @endinternal
 */
void
populate_hosts_file() {
    const auto& hosts_file = GKFS_DATA->hosts_file();
    GKFS_DATA->spdlogger()->debug("{}() Populating hosts file: '{}'", __func__,
                                  hosts_file);
    ofstream lfstream(hosts_file, ios::out | ios::app);
    if(!lfstream) {
        throw runtime_error(fmt::format("Failed to open hosts file '{}': {}",
                                        hosts_file, strerror(errno)));
    }
    // if rootdir_suffix is used, append it to hostname
    auto hostname =
            GKFS_DATA->rootdir_suffix().empty()
                    ? gkfs::rpc::get_my_hostname(true)
                    : fmt::format("{}#{}", gkfs::rpc::get_my_hostname(true),
                                  GKFS_DATA->rootdir_suffix());
    // lxl 写入hostfile
    lfstream << fmt::format("{} {}", hostname, RPC_DATA->self_addr_str())
             << std::endl;
    if(!lfstream) {
        throw runtime_error(
                fmt::format("Failed to write on hosts file '{}': {}",
                            hosts_file, strerror(errno)));
    }
    lfstream.close();
}

/**
 * @internal
 * This function removes the entire hosts file even if just one daemon is
 * shutdown. This makes sense because the data distribution calculation would be
 * misaligned if the entry of the current daemon was only removed.
 * 即使只有一个守护进程关闭，这个函数也会删除整个hosts文件。这是有意义的，因为如果只删除
 * 当前守护进程的条目，则数据分布计算将不一致。
 * @endinternal
 */
void
destroy_hosts_file() {
    std::remove(GKFS_DATA->hosts_file().c_str());
}

} // namespace gkfs::utils
