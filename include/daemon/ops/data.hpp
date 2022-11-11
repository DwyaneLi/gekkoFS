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
 * @brief Classes to encapsulate asynchronous chunk operations.
 * All operations on chunk files must go through the Argobots' task queues.
 * Otherwise operations may overtake operations in the I/O queues.
 * This applies to write, read, and truncate which may modify the middle of a
 * chunk, essentially a write operation.
 *
 * In the future, this class may be used to provide failure tolerance for IO
 * tasks
 *
 * Base class using the CRTP idiom
 */

#ifndef GEKKOFS_DAEMON_DATA_HPP
#define GEKKOFS_DAEMON_DATA_HPP

#include <daemon/daemon.hpp>
#include <common/common_defs.hpp>

#include <string>
#include <vector>

extern "C" {
#include <abt.h>
#include <margo.h>
}

namespace gkfs::data {

/**
 * @brief Internal Exception for all general chunk operations.
 */
class ChunkOpException : public std::runtime_error {
public:
    explicit ChunkOpException(const std::string& s) : std::runtime_error(s){};
};
/**
 * @brief Internal Exception for all chunk write operations.
 */
class ChunkWriteOpException : public ChunkOpException {
public:
    explicit ChunkWriteOpException(const std::string& s)
        : ChunkOpException(s){};
};
/**
 * @brief Internal Exception for all chunk read operations.
 */
class ChunkReadOpException : public ChunkOpException {
public:
    explicit ChunkReadOpException(const std::string& s) : ChunkOpException(s){};
};
/**
 * @brief Internal Exception for all chunk metadata operations.
 */
class ChunkMetaOpException : public ChunkOpException {
public:
    explicit ChunkMetaOpException(const std::string& s) : ChunkOpException(s){};
};
/**
 * @brief Base class (using CRTP idiom) for all chunk operations.
 *
 * This class is not thread-safe.
 * // 不是线程安全的
 * @internal
 * Each I/O operation, i.e., an write or read RPC request, operating on one or
 * multiple chunks is represented by a corresponding ChunkOperation object. To
 * keep conflicting operations of other I/O requests on the same chunk in order,
 * Argobots tasklets are used. Tasklets are lightweight threads compared to
 * User-Level Threads (ULTs). When ULTs run in an ES, their execution may be
 * interleaved inside an ES because they can yield control to the scheduler or
 * another ULT. If this happens during a write, for example, and data is written
 * after one another while sitting in the queue, data might get written in the
 * wrong order. Tasklets are an efficient way to prevent this.
 * 每个I/O操作，例如一个写或读RPC请求，操作一个或多个块由一个相应的ChunkOperation对象表示。
 * 为了保持同一块上其他I/O请求的冲突操作，使用Argobots微线程。与用户级线程(ult)相比，
 * 微线程是轻量级线程。当ULT在ES中运行时，它们的执行可能在ES中交错进行，因为它们可以将
 * 控制权交给调度器或另一个ULT。例如，如果在写入过程中发生这种情况，并且数据在队列中依次写入，
 * 则数据可能以错误的顺序写入。微线程是防止这种情况的有效方法。
 * 
 * Each ChunkOperation includes the path to the directory where all chunks are
 * located, a number of tasks (one for each chunk), and their corresponding
 * eventuals (one for each task). ABT_eventuals offer a similar concept as
 * std::future to provide call-back functionality.
 * 每个ChunkOperation都包含到所有块所在目录的路径、许多任务（可以理解为一个微线程？）(每个块一个)和它们对应的结果(每个任务一个)。
 * ABT_eventuals提供了与std::future类似的概念来提供回调功能。
 * 
 * Truncate requests also create a ChunkOperation since it requires removing a
 * number of chunks and must honor the same order of operations to chunks.
 * 截断请求也会创建ChunkOperation，因为它需要删除许多块，并且必须遵循对块的相同操作顺序。
 * 
 * In the future, additional optimizations can be made since atomicity of the
 * tasklets might be too long if they depend on the results of a, e.g., pread().
 * Therefore, a queue per chunk could be beneficial (this has not been tested
 * yet).
 * 在未来，可以进行额外的优化，因为如果微线程依赖于a的结果，它们的原子性可能太长，例如pread()。
 * 因此，每个块一个队列可能是有益的(这还没有经过测试)。
 * 
 * Note, at this time, CRTP is only required for `cancel_all_tasks()`.
 * CRTP只用于cancel_all_tasks
 * @endinternal
 * @tparam OperationType for write, read, and truncate.
 */
template <class OperationType>
class ChunkOperation {

protected:
    // chunk所在的目录，也就是最后一级是文件名
    const std::string path_; //!< Path to the chunk directory of the file

    std::vector<ABT_task> abt_tasks_; //!< Tasklets operating on the file 对文件进行操作的微线程
    std::vector<ABT_eventual>
            task_eventuals_; //!< Eventuals for tasklet callbacks 用于微线程回调的Eventuals

public:
    /**
     * @brief Constructor for a single chunk operation
     * 构造函数. 默认对一个块进行操作
     * @param path Path to chunk directory
     */
    explicit ChunkOperation(const std::string& path)
        : ChunkOperation(path, 1){};

    /**
     * @brief Constructor to initialize tasklet and eventual lists.
     * @param path Path to chunk directory
     * @param n Number of chunk operations by I/O request
     */
    ChunkOperation(std::string path, size_t n) : path_(std::move(path)) {
        // Knowing n beforehand is important and cannot be dynamic. Otherwise
        // eventuals cause seg faults
        abt_tasks_.resize(n);
        task_eventuals_.resize(n);
    };
    /**
     * Destructor calls cancel_all_tasks to clean up all used resources.
     */
    ~ChunkOperation() {
        cancel_all_tasks();
    }

    /**
     * @brief Cancels all tasks in-flight and free resources.
     * 取消所有 正在运行？ 中的任务和免费资源。
     */
    void
    cancel_all_tasks() {
        GKFS_DATA->spdlogger()->trace("{}() enter", __func__);
        for(auto& task : abt_tasks_) {
            if(task) {
                ABT_task_cancel(task);
                ABT_task_free(&task);
            }
        }
        for(auto& eventual : task_eventuals_) {
            if(eventual) {
                ABT_eventual_reset(eventual);
                ABT_eventual_free(&eventual);
            }
        }
        abt_tasks_.clear();
        task_eventuals_.clear();
        static_cast<OperationType*>(this)->clear_task_args();
    }
};

/**
 * @brief Chunk operation class for truncate operations.
 *
 * Note, a truncate operation is a special case and forced to only use a single
 * task.
 * 截断操作是一种特殊情况，只能使用单个任务。
 */
class ChunkTruncateOperation : public ChunkOperation<ChunkTruncateOperation> {
    friend class ChunkOperation<ChunkTruncateOperation>;
    // 友元类，那么ChunkOperation<ChunkTruncateOperation>类可以访问ChunkTruncateOperation所有成员，
    // 所以上面 cancel_all_tasks()才能调用clear_task_args()
private:
    struct chunk_truncate_args {
        const std::string* path; //!< Path to affected chunk directory 受影响的块目录的路径
        size_t size; //!< GekkoFS file offset (_NOT_ chunk file) to truncate to
        ABT_eventual eventual; //!< Attached eventual 相关联的回调
    };                         //!< Struct for a truncate operation 截断操作的结构

    struct chunk_truncate_args task_arg_ {}; //!< tasklet input struct 微线程输入结构
    /**
     * @brief Exclusively used by the Argobots tasklet. 由Argobots微线程独家使用
     * @param _arg Pointer to input struct of type <chunk_truncate_args>. Error
     * code<int> is placed into eventual to signal its failure or success.
     * arg指向类型<chunk_truncate_args>的输入结构体的指针。</chunk_truncate_args>将错误代码<int>放入变量eventual中，表示失败或成功。</int>
     */
    static void
    truncate_abt(void* _arg); 
    /**
     * @brief Resets the task_arg_ struct.
     */
    void
    clear_task_args();

public:
    explicit ChunkTruncateOperation(const std::string& path);

    ~ChunkTruncateOperation() = default;

    /**
     * @brief Truncate request called by RPC handler function and launches a
     * non-blocking tasklet.
     * 由RPC处理器函数调用的请求，并启动一个非阻塞微线程。
     * @param size GekkoFS file offset (_NOT_ chunk file) to truncate to
     * @throws ChunkMetaOpException
     */
    void
    truncate(size_t size);
    /**
     * @brief Wait for the truncate tasklet to finish.
     * @return Error code for success (0) or failure
     */
    int
    wait_for_task();
};

/**
 * @brief Chunk operation class for write operations with one object per write
 * RPC request. May involve multiple I/O task depending on the number of chunks
 * involved.
 * 块操作类，用于每个写RPC请求具有一个这样的对象。可能涉及多个I/O任务，取决于所涉及的块的数量。
 * 就是可以说一个写操作对应一个类实例，并且一个类实例对应多个块
 */
class ChunkWriteOperation : public ChunkOperation<ChunkWriteOperation> {
    friend class ChunkOperation<ChunkWriteOperation>;

private:
    struct chunk_write_args {
        const std::string* path;      //!< Path to affected chunk directory
        const char* buf;              //!< Buffer for chunk
        gkfs::rpc::chnk_id_t chnk_id; //!< chunk id that is affected
        size_t size;                  //!< size to write for chunk
        off64_t off;                  //!< offset for individual chunk
        ABT_eventual eventual;        //!< Attached eventual
    };                                //!< Struct for an chunk write operation

    std::vector<struct chunk_write_args> task_args_; //!< tasklet input structs 和上面那个不一样，这个是多个任务，截断只有一个
    /**
     * @brief Exclusively used by the Argobots tasklet.
     * @param _arg Pointer to input struct of type <chunk_write_args>. Error
     * code<int> is placed into eventual to signal its failure or success.
     */
    static void
    write_file_abt(void* _arg);
    /**
     * @brief Resets the task_arg_ struct.
     */
    void
    clear_task_args();

public:
    ChunkWriteOperation(const std::string& path, size_t n);

    ~ChunkWriteOperation() = default;

    /**
     * @brief Write request called by RPC handler function and launches a
     * non-blocking tasklet.
     * 由RPC处理器函数调用的请求，并启动一个非阻塞微线程
     * @param idx Number of non-blocking write for write RPC request
     * @param chunk_id The affected chunk id
     * @param bulk_buf_ptr The buffer to write for the chunk
     * @param size Size to write for chunk
     * @param offset Offset for individual chunk
     * @throws ChunkWriteOpException
     */
    void
    write_nonblock(size_t idx, uint64_t chunk_id, const char* bulk_buf_ptr,
                   size_t size, off64_t offset);

    /**
     * @brief Wait for all write tasklets to finish.
     * @return Pair for error code for success (0) or failure and written size
     */
    std::pair<int, size_t>
    wait_for_tasks();
};

/**
 * @brief Chunk operation class for read operations with one object per read
 * RPC request. May involve multiple I/O task depending on the number of chunks
 * involved.
 * 和上面的写类似，多了一个bulk_args
 */
class ChunkReadOperation : public ChunkOperation<ChunkReadOperation> {
    friend class ChunkOperation<ChunkReadOperation>;

private:
    struct chunk_read_args {
        const std::string* path;      //!< Path to affected chunk directory
        char* buf;                    //!< Buffer for chunk
        gkfs::rpc::chnk_id_t chnk_id; //!< chunk id that is affected
        size_t size;                  //!< size to read from chunk
        off64_t off;                  //!< offset for individual chunk
        ABT_eventual eventual;        //!< Attached eventual
    };                                //!< Struct for an chunk read operation

    std::vector<struct chunk_read_args> task_args_; //!< tasklet input structs
    /**
     * @brief Exclusively used by the Argobots tasklet.
     * @param _arg Pointer to input struct of type <chunk_read_args>. Error
     * code<int> is placed into eventual to signal its failure or success.
     */
    static void
    read_file_abt(void* _arg);
    /**
     * @brief Resets the task_arg_ struct.
     */
    void
    clear_task_args();

public:
    struct bulk_args {
        margo_instance_id mid;               //!< Margo instance ID of server
        hg_addr_t origin_addr;               //!< abstract address of client
        hg_bulk_t origin_bulk_handle;        //!< bulk handle from client
        std::vector<size_t>* origin_offsets; //!< offsets in origin buffer
        hg_bulk_t local_bulk_handle;         //!< local bulk handle for PUSH
        std::vector<size_t>* local_offsets;  //!< offsets in local buffer
        std::vector<uint64_t>* chunk_ids;    //!< all chunk ids in this read
    }; //!< Struct to push read data to the client

    ChunkReadOperation(const std::string& path, size_t n);

    ~ChunkReadOperation() = default;

    /**
     * @brief Read request called by RPC handler function and launches a
     * non-blocking tasklet.
     * @param idx Number of non-blocking write for write RPC request
     * @param chunk_id The affected chunk id
     * @param bulk_buf_ptr The buffer for reading chunk
     * @param size Size to read from chunk
     * @param offset Offset for individual chunk
     * @throws ChunkReadOpException
     */
    void
    read_nonblock(size_t idx, uint64_t chunk_id, char* bulk_buf_ptr,
                  size_t size, off64_t offset);

    /**
     * @brief Waits for all local I/O operations to finish and push buffers back
     * to the daemon.
     * @param args Bulk_args for push transfer
     * @return Pair for error code for success (0) or failure and read size
     */
    std::pair<int, size_t>
    wait_for_tasks_and_push_back(const bulk_args& args);
};

} // namespace gkfs::data

#endif // GEKKOFS_DAEMON_DATA_HPP
