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

#include <client/path.hpp>
#include <client/preload.hpp>
#include <client/logging.hpp>
#include <client/env.hpp>

#include <common/path_util.hpp>

#include <vector>
#include <string>
#include <cassert>
#include <climits>

extern "C" {
#include <sys/stat.h>
#include <libsyscall_intercept_hook_point.h>
}

using namespace std;

namespace gkfs::path {

static const string excluded_paths[2] = {"sys/", "proc/"};

/** Match components in path
 *
 * Returns the number of consecutive components at start of `path`
 * that match the ones in `components` vector.
 *
 * `path_components` will be set to the total number of components found in
 * `path`
 *
 * Example:
 * ```ÏÏ
 *  unsigned int tot_comp;
 *  path_match_components("/matched/head/with/tail", &tot_comp, ["matched",
 * "head", "no"]) == 2; tot_comp == 4;
 * ```
 */
// components匹配，返回匹配数
unsigned int
match_components(const string& path, unsigned int& path_components,
                 const ::vector<string>& components) {
    unsigned int matched = 0;
    unsigned int processed_components = 0;
    string::size_type comp_size = 0; // size of current component
    string::size_type start = 0;     // start index of curr component
    string::size_type end = 0; // end index of curr component (last processed
                               // Path Separator "separator")

    while(++end < path.size()) {
        start = end;

        // Find next component
        end = path.find(path::separator, start);
        if(end == string::npos) {
            end = path.size();
        }

        comp_size = end - start;
        if(matched == processed_components &&
           path.compare(start, comp_size, components.at(matched)) == 0) {
            ++matched;
        }
        ++processed_components;
    }
    path_components = processed_components;
    return matched;
}

/** Resolve path to its canonical representation
 *
 * Populate `resolved` with the canonical representation of `path`.
 *
 * ".", ".." and symbolic links gets resolved.
 *
 * If `resolve_last_link` is false, the last components in path
 * won't be resolved if its a link.
 *
 * returns true if the resolved path fall inside GekkoFS namespace,
 * and false otherwise.
 */
// 将path的规范形式填充到resolved里，如果是gekkofs里的返回true
// 所谓规范就是去除了挂载路径，也就是相对于挂载路径的路径
bool
resolve(const string& path, string& resolved, bool resolve_last_link) {

    LOG(DEBUG, "path: \"{}\", resolved: \"{}\", resolve_last_link: {}", path,
        resolved, resolve_last_link);

    assert(path::is_absolute(path));
    // 在sys和在proc下的不是gekkofs
    for(auto& excl_path : excluded_paths) {
        if(path.compare(1, excl_path.length(), excl_path) == 0) {
            LOG(DEBUG, "Skipping: '{}'", path);
            resolved = path;
            return false;
        }
    }

    struct stat st {};
    // 挂载了文件系统的目录
    const ::vector<string>& mnt_components = CTX->mountdir_components();
    unsigned int matched_components =
            0; // matched number of component in mountdir 与挂载目录匹配的组件数
    unsigned int resolved_components = 0; //resolved 中components的数量
    string::size_type comp_size = 0; // size of current component 现在的组件的长度
    string::size_type start = 0;     // start index of curr component 现在的组件的起始index
    string::size_type end = 0; // end index of curr component (last processed
                               // Path Separator "separator") curr成分的结束索引(最后处理的路径分隔符" Separator ")
    string::size_type last_slash_pos =
            0; // index of last slash in resolved path 已解析路径中最后一个斜杠的索引
    resolved.clear();
    resolved.reserve(path.size());

    while(++end < path.size()) {
        start = end;

        /* Skip sequence of multiple path-separators. */
        // 跳过多个路径分隔符的序列。
        while(start < path.size() && path[start] == path::separator) {
            ++start;
        }

        // Find next component
        end = path.find(path::separator, start);
        if(end == string::npos) {
            end = path.size();
        }
        comp_size = end - start;

        if(comp_size == 0) {
            // component is empty (this must be the last component)
            break;
        }
        if(comp_size == 1 && path.at(start) == '.') {
            // component is '.', we skip it
            continue;
        }
        if(comp_size == 2 && path.at(start) == '.' &&
           path.at(start + 1) == '.') {
            // component is '..' we need to rollback resolved path
            if(!resolved.empty()) {
                resolved.erase(last_slash_pos);
                /* TODO     Optimization
                 * the previous slash position should be stored.
                 * The following search could be avoided.
                 */
                last_slash_pos = resolved.find_last_of(path::separator);
            }
            if(resolved_components > 0) {
                if(matched_components == resolved_components) {
                    --matched_components;
                }
                --resolved_components;
            }
            continue;
        }

        // add `/<component>` to the reresolved path
        resolved.push_back(path::separator);
        last_slash_pos = resolved.size() - 1;
        resolved.append(path, start, comp_size);

        if(matched_components < mnt_components.size()) {
            // Outside GekkoFS
            // 这个component匹配成功
            if(matched_components == resolved_components &&
               path.compare(start, comp_size,
                            mnt_components.at(matched_components)) == 0) {
                ++matched_components;
            }
            // 路径不存在
            if(lstat(resolved.c_str(), &st) < 0) {

                LOG(DEBUG, "path \"{}\" does not exist", resolved);

                resolved.append(path, end, string::npos);
                return false;
            }
            if(S_ISLNK(st.st_mode)) {
                if(!resolve_last_link && end == path.size()) {
                    continue;
                }
                auto link_resolved = ::unique_ptr<char[]>(new char[PATH_MAX]);
                if(realpath(resolved.c_str(), link_resolved.get()) == nullptr) {

                    LOG(ERROR,
                        "Failed to get realpath for link \"{}\". "
                        "Error: {}",
                        resolved, ::strerror(errno));

                    resolved.append(path, end, string::npos);
                    return false;
                }
                // substituute resolved with new link path
                resolved = link_resolved.get();
                matched_components = match_components(
                        resolved, resolved_components, mnt_components);
                // set matched counter to value coherent with the new path
                last_slash_pos = resolved.find_last_of(path::separator);
                continue;
            } else if((!S_ISDIR(st.st_mode)) && (end != path.size())) {
                // 不是目录，是文件且长度不对，则退出
                resolved.append(path, end, string::npos);
                return false;
            }
        } else {
            // Inside GekkoFS
            ++matched_components;
        }
        ++resolved_components;
    }

    if(matched_components >= mnt_components.size()) {
        resolved.erase(1, CTX->mountdir().size());
        LOG(DEBUG, "internal: \"{}\"", resolved);
        return true;
    }

    if(resolved.empty()) {
        resolved.push_back(path::separator);
    }
    LOG(DEBUG, "external: \"{}\"", resolved);
    return false;
}

// 查看当前路径
string
get_sys_cwd() {
    char temp[path::max_length];
    if(long ret =
               syscall_no_intercept(SYS_getcwd, temp, path::max_length) < 0) {
        throw ::system_error(syscall_error_code(ret), ::system_category(),
                             "Failed to retrieve current working directory");
    }
    // getcwd could return "(unreachable)<PATH>" in some cases
    if(temp[0] != path::separator) {
        throw ::runtime_error("Current working directory is unreachable");
    }
    return {temp};
}

// 切换当前工作路径
void
set_sys_cwd(const string& path) {

    LOG(DEBUG, "Changing working directory to \"{}\"", path);

    if(long ret = syscall_no_intercept(SYS_chdir, path.c_str())) {
        LOG(ERROR, "Failed to change working directory: {}",
            ::strerror(syscall_error_code(ret)));
        throw ::system_error(syscall_error_code(ret), ::system_category(),
                             "Failed to set system current working directory");
    }
}

// 将环境变量LIBDKFS_CWD设置为path
void
set_env_cwd(const string& path) {

    LOG(DEBUG, "Setting {} to \"{}\"", gkfs::env::CWD, path);

    if(setenv(gkfs::env::CWD, path.c_str(), 1)) {
        LOG(ERROR, "Failed while setting {}: {}", gkfs::env::CWD,
            ::strerror(errno));
        throw ::system_error(
                errno, ::system_category(),
                "Failed to set environment current working directory");
    }
}

void
unset_env_cwd() {

    LOG(DEBUG, "Clearing {}()", gkfs::env::CWD);

    if(unsetenv(gkfs::env::CWD)) {

        LOG(ERROR, "Failed to clear {}: {}", gkfs::env::CWD, ::strerror(errno));

        throw ::system_error(
                errno, ::system_category(),
                "Failed to unset environment current working directory");
    }
}

// 初始化cwd，如果已经设置了env_cwd,env_cwd就是cwd，不然就是sys_cwd
void
init_cwd() {
    const char* env_cwd = ::getenv(gkfs::env::CWD);
    if(env_cwd != nullptr) {
        CTX->cwd(env_cwd);
    } else {
        CTX->cwd(get_sys_cwd());
    }
}

// 设置cwd，cwd会被设置成path
void
set_cwd(const string& path, bool internal) {
    if(internal) {
        set_sys_cwd(CTX->mountdir());
        set_env_cwd(path);
    } else {
        set_sys_cwd(path);
        unset_env_cwd();
    }
    // 上面的设置在下次init_cwd的时候使用，下次init之后，cwd仍是path，internal用来区别是否设置环境变量
    CTX->cwd(path);
}

} // namespace gkfs::path