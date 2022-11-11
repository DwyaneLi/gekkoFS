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
 * @brief Chunk storage declarations handles all interactions with the
 * node-local storage system.
 */

// lxl 其实都是对本地文件的处理了，就是用库函数处理文件
#ifndef GEKKOFS_CHUNK_STORAGE_HPP
#define GEKKOFS_CHUNK_STORAGE_HPP

#include <common/common_defs.hpp>

#include <limits>
#include <string>
#include <memory>
#include <system_error>

/* Forward declarations */
namespace spdlog {
class logger;
}

namespace gkfs::data {

struct ChunkStat {
    unsigned long chunk_size;
    unsigned long chunk_total;
    unsigned long chunk_free;
}; //!< Struct for attaining current usage of storage backend

/**
 * @brief Generic exception for ChunkStorage
 */
class ChunkStorageException : public std::system_error {
public:
    ChunkStorageException(const int err_code, const std::string& s)
        : std::system_error(err_code, std::generic_category(), s){};
};

/**
 * @brief ChunkStorage class handles _all_ interaction with node-local storage
 * system and is run as a single instance within the GekkoFS daemon.
 * ChunkStorage类处理与节点本地存储系统的所有交互，并在GekkoFS守护进程中作为单个实例运行。
 */
class ChunkStorage {
private:
    std::shared_ptr<spdlog::logger> log_; //!< Class logger

    std::string root_path_; //!< Path to GekkoFS root directory
    size_t chunksize_; //!< File system chunksize. TODO Why does that exist?

    /**
     * @brief Converts an internal gkfs path under the root dir to the absolute
     * path of the system.
     * 将根目录下的内部gkfs路径转换为系统的绝对路径。
     * @param internal_path E.g., /foo/bar
     * @return Absolute path, e.g., /tmp/rootdir/<pid>/data/chunks/foo:bar
     */
    [[nodiscard]] inline std::string
    absolute(const std::string& internal_path) const;

    /**
     * @brief Returns the chunk dir directory for a given path which is expected
     * to be absolute.
     * 返回给定路径的chunk dir目录，该路径应该是绝对路径。
     * @param file_path
     * @return Chunk dir path
     */
    static inline std::string
    get_chunks_dir(const std::string& file_path);

    /**
     * @brief Returns the backend chunk file path for a given internal path.
     * 返回给定内部路径的后端块文件路径。
     * @param file_path Internal file path, e.g., /foo/bar
     * @param chunk_id Number of chunk id
     * @return Chunk file path, e.g., /foo/bar
     * /tmp/rootdir/<pid>>/data/chunks/foo:bar/0
     */
    static inline std::string
    get_chunk_path(const std::string& file_path, gkfs::rpc::chnk_id_t chunk_id);

    /**
     * @brief Initializes the chunk space for a GekkoFS file, creating its
     * directory on the local file system.
     * 初始化GekkoFS文件的块空间，在本地文件系统上创建其目录。
     * @param file_path Chunk file path, e.g., /foo/bar
     */
    void
    init_chunk_space(const std::string& file_path) const;

public:
    /**
     * @brief Initializes the ChunkStorage object on daemon launch.
     * 在守护进程启动时初始化ChunkStorage对象。
     * @param path Root directory where all data is placed on the local FS.
     * @param chunksize Used chunksize in this GekkoFS instance.
     * @throws ChunkStorageException on launch failure
     */
    ChunkStorage(std::string& path, size_t chunksize);

    /**
     * @brief Removes chunk directory with all its files which is a recursive
     * remove operation on the chunk directory.
     * 删除块目录及其所有文件，这是块目录上的递归删除操作。
     * @param file_path Chunk file path, e.g., /foo/bar
     * @throws ChunkStorageException
     */
    void
    destroy_chunk_space(const std::string& file_path) const;

    /**
     * @brief Writes a single chunk file and is usually called by an Argobots
     * tasklet.
     * 写入单个块文件，通常由Argobots微线程调用。
     * @param file_path Chunk file path, e.g., /foo/bar
     * @param chunk_id Number of chunk id
     * @param buf Buffer to write to chunk
     * @param size Amount of bytes to write to the chunk file
     * @param offset Offset where to write to the chunk file
     * @return The amount of bytes written
     * @throws ChunkStorageException with its error code
     */
    ssize_t
    write_chunk(const std::string& file_path, gkfs::rpc::chnk_id_t chunk_id,
                const char* buf, size_t size, off64_t offset) const;

    /**
     * @brief Reads a single chunk file and is usually called by an Argobots
     * tasklet.
     * 读取单个块文件，通常由Argobots微线程调用。
     * @param file_path Chunk file path, e.g., /foo/bar
     * @param chunk_id Number of chunk id
     * @param buf Buffer to read to from chunk
     * @param size Amount of bytes to read to the chunk file
     * @param offset Offset where to read from the chunk file
     * @return The amount of bytes read
     * @throws ChunkStorageException with its error code
     */
    ssize_t
    read_chunk(const std::string& file_path, gkfs::rpc::chnk_id_t chunk_id,
               char* buf, size_t size, off64_t offset) const;

    /**
     * @brief Delete all chunks starting with chunk a chunk id.
     * 删除所有chunk 从 chunk id开始。
     * @param file_path Chunk file path, e.g., /foo/bar
     * @param chunk_start Number of chunk id
     * @throws ChunkStorageException with its error code
     */
    void
    trim_chunk_space(const std::string& file_path,
                     gkfs::rpc::chnk_id_t chunk_start);

    /**
     * @brief Truncates a single chunk file to a given byte length.
     * 将单个块文件截断为给定的字节长度。
     * @param file_path Chunk file path, e.g., /foo/bar
     * @param chunk_id Number of chunk id
     * @param length Length of bytes to truncate the chunk to // 新长度
     * @throws ChunkStorageException
     */
    void
    truncate_chunk_file(const std::string& file_path,
                        gkfs::rpc::chnk_id_t chunk_id, off_t length);

    /**
     * @brief Calls statfs on the chunk directory to get statistic on its used
     * storage space.
     * 调用块目录上的statfs以获取其已使用存储空间的统计信息。
     * @return ChunkStat struct
     * @throws ChunkStorageException
     */
    [[nodiscard]] ChunkStat
    chunk_stat() const;
};

} // namespace gkfs::data

#endif // GEKKOFS_CHUNK_STORAGE_HPP
