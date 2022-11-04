/*
  Copyright 2018-2022, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2022, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  This file is part of GekkoFS' POSIX interface.

  GekkoFS' POSIX interface is free software: you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the License,
  or (at your option) any later version.

  GekkoFS' POSIX interface is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with GekkoFS' POSIX interface.  If not, see
  <https://www.gnu.org/licenses/>.

  SPDX-License-Identifier: LGPL-3.0-or-later
*/

#ifndef GEKKOFS_OPEN_FILE_MAP_HPP
#define GEKKOFS_OPEN_FILE_MAP_HPP

#include <map>
#include <mutex>
#include <memory>
#include <atomic>
#include <array>
// openfile 里的路径全都是相对于挂载目录的路径
namespace gkfs::filemap {

/* Forward declaration */
class OpenDir;


enum class OpenFile_flags {
    append = 0,
    creat,
    trunc,
    rdonly, // 只读
    wronly, // 只写
    rdwr,   // 可读可写
    cloexec, // close on excel
    flag_count // this is purely used as a size variable of this enum class // 用于统计数量的
};

enum class FileType { regular, directory };

class OpenFile {
protected:
    FileType type_;
    std::string path_;
    std::array<bool, static_cast<int>(OpenFile_flags::flag_count)> flags_ = {
            {false}};
    unsigned long pos_;
    // 互斥锁
    std::mutex pos_mutex_;
    std::mutex flag_mutex_;

public:
    // multiple threads may want to update the file position if fd has been
    // duplicated by dup()

    OpenFile(const std::string& path, int flags,
             FileType type = FileType::regular);

    ~OpenFile() = default;

    // getter/setter
    std::string
    path() const;

    void
    path(const std::string& path_);

    unsigned long
    pos();

    void
    pos(unsigned long pos_);

    bool
    get_flag(OpenFile_flags flag);

    void
    set_flag(OpenFile_flags flag, bool value);

    FileType
    type() const;
};


class OpenFileMap {

private:
    std::map<int, std::shared_ptr<OpenFile>> files_; // 前面的int应该就是文件描述符
    std::recursive_mutex files_mutex_;

    int
    safe_generate_fd_idx_();

    /*
     * TODO: Setting our file descriptor index to a specific value is dangerous
     * because we might clash with the kernel. E.g., if we would passthrough and
     * not intercept and the kernel assigns a file descriptor but we will later
     * use the same fd value, we will intercept calls that were supposed to be
     * going to the kernel. This works the other way around too. To mitigate
     * this issue, we set the initial fd number to a high value. We "hope" that
     * we do not clash but this is no permanent solution. Note: This solution
     * will probably work well already for many cases because kernel fd values
     * are reused, unlike to ours. The only case where we will clash with the
     * kernel is, if one process has more than 100000 files open at the same
     * time.
     * TODO:将文件描述符索引设置为特定值是危险的，因为可能会与内核发生冲突。例如，
     * 如果我们是传递而不是拦截，并且内核分配了一个文件描述符，但我们稍后将使用相
     * 同的fd值，那么我们将拦截本该到达内核的调用。反过来也一样。为了缓解这个问题，
     * 我们将初始fd数设置为一个较高的值。我们“希望”我们不会发生冲突，但这不是永久
     * 的解决方案。注意:这个解决方案可能已经在很多情况下很好地工作了，因为内核fd值
     * 是重用的，不像我们的。惟一会与内核发生冲突的情况是，一个进程同时打开的文件超过100000个。
     */

    int fd_idx;
    std::mutex fd_idx_mutex;
    std::atomic<bool> fd_validation_needed;

public:
    OpenFileMap();

    std::shared_ptr<OpenFile>
    get(int fd);

    std::shared_ptr<OpenDir>
    get_dir(int dirfd);

    bool
    exist(int fd);

    int add(std::shared_ptr<OpenFile>);

    bool
    remove(int fd);

    int
    dup(int oldfd);

    int
    dup2(int oldfd, int newfd);

    int
    generate_fd_idx();

    int
    get_fd_idx();
};

} // namespace gkfs::filemap

#endif // GEKKOFS_OPEN_FILE_MAP_HPP
