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

#ifndef FS_METADATA_H
#define FS_METADATA_H
#pragma once


#include <config.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>

namespace gkfs::metadata {

constexpr mode_t LINK_MODE = ((S_IRWXU | S_IRWXG | S_IRWXO) | S_IFLNK);

class Metadata {
private:
    time_t atime_{}; // access time. gets updated on file access unless mounted
                     // with noatime 访问时间
    time_t mtime_{}; // modify time. gets updated when file content is modified. 修改时间（文件内容被修改的时间）
    time_t ctime_{}; // change time. gets updated when the file attributes are
                     // changed AND when file content is modified. 改变时间（文件内容和属性被修改的时间）
    mode_t mode_{};  // 文件属性位掩码的类型
    nlink_t link_count_{}; // number of names for this inode (hardlinks) inode硬连接的数量
    size_t size_{};     // size_ in bytes, might be computed instead of stored  文件大小
    blkcnt_t blocks_{}; // allocated file system blocks_ 文件块的数量
#ifdef HAS_SYMLINKS
    std::string target_path_; // For links this is the path of the target file 对于链接，这是目标文件的路径
#endif


public:
    Metadata() = default;

    explicit Metadata(mode_t mode);

#ifdef HAS_SYMLINKS

    Metadata(mode_t mode, const std::string& target_path);

#endif

    // Construct from a binary representation of the object 从对象的二进制表示形式构造
    explicit Metadata(const std::string& binary_str);

    // 用于元数据的序列化，序列化为一个字符串， 其中的设置和配置文件有关（config.hpp）
    std::string
    serialize() const;

    void
    init_ACM_time();

    // 更新时间，哪个bool为真更新哪一个
    void
    update_ACM_time(bool a, bool c, bool m);

    // Getter and Setter
    time_t
    atime() const;

    void
    atime(time_t atime_);

    time_t
    mtime() const;

    void
    mtime(time_t mtime_);

    time_t
    ctime() const;

    void
    ctime(time_t ctime_);

    mode_t
    mode() const;

    void
    mode(mode_t mode_);

    nlink_t
    link_count() const;

    void
    link_count(nlink_t link_count_);

    size_t
    size() const;

    void
    size(size_t size_);

    blkcnt_t
    blocks() const;

    void
    blocks(blkcnt_t blocks_);

#ifdef HAS_SYMLINKS

    std::string
    target_path() const;

    void
    target_path(const std::string& target_path);

    bool
    is_link() const;

#endif
};

} // namespace gkfs::metadata


#endif // FS_METADATA_H
