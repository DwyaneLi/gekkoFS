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

#ifndef GEKKOFS_RPC_DISTRIBUTOR_HPP
#define GEKKOFS_RPC_DISTRIBUTOR_HPP

#include "../include/config.hpp"
#include <vector>
#include <string>
#include <numeric>
#include <unordered_map>
#include <fstream>
#include <map>
// 这一部分和hash有关 distribute分散
namespace gkfs::rpc {

using chunkid_t = unsigned int;
using host_t = unsigned int;

class Distributor {
public:
    // 下面这些函数用于定位数据和metadata
    virtual host_t
    localhost() const = 0;

    virtual host_t
    locate_data(const std::string& path, const chunkid_t& chnk_id) const = 0;
    // TODO: We need to pass hosts_size in the server side, because the number
    // of servers are not defined (in startup)
    // 启动的时候需要用这个函数定位，因为一开始并没有传递host_size,这个参数host_size会更新成员变量
    virtual host_t
    locate_data(const std::string& path, const chunkid_t& chnk_id,
                unsigned int hosts_size) = 0;

    virtual host_t
    locate_file_metadata(const std::string& path) const = 0;

    virtual std::vector<host_t>
    locate_directory_metadata(const std::string& path) const = 0;
};


class SimpleHashDistributor : public Distributor {
private:
    host_t localhost_; // 本地主机
    unsigned int hosts_size_{0}; // 主机数量
    std::vector<host_t> all_hosts_; // 所有主机
    std::hash<std::string> str_hash;

public:
    SimpleHashDistributor();

    SimpleHashDistributor(host_t localhost, unsigned int hosts_size);

    host_t
    localhost() const override;

    host_t
    locate_data(const std::string& path,
                const chunkid_t& chnk_id) const override;

    host_t
    locate_data(const std::string& path, const chunkid_t& chnk_id,
                unsigned int host_size);

    host_t
    locate_file_metadata(const std::string& path) const override;

    // ？？？ 为什么对于目录元数据就是返回所有host？
    std::vector<host_t>
    locate_directory_metadata(const std::string& path) const override;
};

// 本地唯一的结点，自然所有数据都在本地
class LocalOnlyDistributor : public Distributor {
private:
    host_t localhost_;

public:
    explicit LocalOnlyDistributor(host_t localhost);

    host_t
    localhost() const override;

    host_t
    locate_data(const std::string& path,
                const chunkid_t& chnk_id) const override;

    host_t
    locate_file_metadata(const std::string& path) const override;

    std::vector<host_t>
    locate_directory_metadata(const std::string& path) const override;
};

// 数据在本节点， 元数据哈希？ 和server有关？
class ForwarderDistributor : public Distributor {
private:
    host_t fwd_host_;
    unsigned int hosts_size_;
    std::vector<host_t> all_hosts_;
    std::hash<std::string> str_hash;

public:
    ForwarderDistributor(host_t fwhost, unsigned int hosts_size);

    host_t
    localhost() const override final;

    host_t
    locate_data(const std::string& path,
                const chunkid_t& chnk_id) const override final;

    host_t
    locate_file_metadata(const std::string& path) const override;

    std::vector<host_t>
    locate_directory_metadata(const std::string& path) const override;
};

// 块区间？
/*
 * Class IntervalSet
 * FROM
 *https://stackoverflow.com/questions/55646605/is-there-a-collection-for-storing-discrete-intervals
 **/
class IntervalSet {
    std::map<chunkid_t, chunkid_t> _intervals;

public:
    void Add(chunkid_t, chunkid_t);
    bool
    IsInsideInterval(unsigned int) const;
};

// 我感觉这个有点类似于导航了
class GuidedDistributor : public Distributor {
private:
    host_t localhost_;
    unsigned int hosts_size_{0};
    std::vector<host_t> all_hosts_;
    std::hash<std::string> str_hash;
    // 最主要的数据结构
    // string：文件路径
    // map_interval: key：IntervalSet 文件对应的chunk块  value: 所在结点号
    std::unordered_map<std::string, std::pair<IntervalSet, unsigned int>>
            map_interval;
    // 记录了一些前缀（也可以理解成目录？）
    std::vector<std::string> prefix_list; // Should not be very long
    bool
    init_guided();

public:
    GuidedDistributor();

    GuidedDistributor(host_t localhost, unsigned int hosts_size);

    host_t
    localhost() const override;

    host_t
    locate_data(const std::string& path,
                const chunkid_t& chnk_id) const override;

    host_t
    locate_data(const std::string& path, const chunkid_t& chnk_id,
                unsigned int host_size);

    host_t
    locate_file_metadata(const std::string& path) const override;

    std::vector<host_t>
    locate_directory_metadata(const std::string& path) const override;
};

} // namespace gkfs::rpc

#endif // GEKKOFS_RPC_LOCATOR_HPP
