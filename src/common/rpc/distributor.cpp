/*
  Copyright 2018-2021, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2021, Johannes Gutenberg Universitaet Mainz, Germany

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

#include <common/rpc/distributor.hpp>

using namespace std;

namespace gkfs {

namespace rpc {

SimpleHashDistributor::SimpleHashDistributor(host_t localhost,
                                             unsigned int hosts_size)
    : localhost_(localhost), hosts_size_(hosts_size), all_hosts_(hosts_size) {
    ::iota(all_hosts_.begin(), all_hosts_.end(), 0);
}

SimpleHashDistributor::SimpleHashDistributor() {}

host_t
SimpleHashDistributor::localhost() const {
    return localhost_;
}

host_t
SimpleHashDistributor::locate_data(const string& path,
                                   const chunkid_t& chnk_id) const {
    return str_hash(path + ::to_string(chnk_id)) % hosts_size_;
}

host_t
SimpleHashDistributor::locate_data(const string& path, const chunkid_t& chnk_id,
                                   unsigned int hosts_size) {
    if(hosts_size_ != hosts_size) {
        hosts_size_ = hosts_size;
        all_hosts_ = std::vector<unsigned int>(hosts_size);
        ::iota(all_hosts_.begin(), all_hosts_.end(), 0);
    }

    return str_hash(path + ::to_string(chnk_id)) % hosts_size_;
}

host_t
SimpleHashDistributor::locate_file_metadata(const string& path) const {
    return str_hash(path) % hosts_size_;
}

::vector<host_t>
SimpleHashDistributor::locate_directory_metadata(const string& path) const {
    return all_hosts_;
}

LocalOnlyDistributor::LocalOnlyDistributor(host_t localhost)
    : localhost_(localhost) {}

host_t
LocalOnlyDistributor::localhost() const {
    return localhost_;
}

host_t
LocalOnlyDistributor::locate_data(const string& path,
                                  const chunkid_t& chnk_id) const {
    return localhost_;
}

host_t
LocalOnlyDistributor::locate_file_metadata(const string& path) const {
    return localhost_;
}

::vector<host_t>
LocalOnlyDistributor::locate_directory_metadata(const string& path) const {
    return {localhost_};
}

ForwarderDistributor::ForwarderDistributor(host_t fwhost,
                                           unsigned int hosts_size)
    : fwd_host_(fwhost), hosts_size_(hosts_size), all_hosts_(hosts_size) {
    ::iota(all_hosts_.begin(), all_hosts_.end(), 0);
}

host_t
ForwarderDistributor::localhost() const {
    return fwd_host_;
}

host_t
ForwarderDistributor::locate_data(const std::string& path,
                                  const chunkid_t& chnk_id) const {
    return fwd_host_;
}

host_t
ForwarderDistributor::locate_file_metadata(const std::string& path) const {
    return str_hash(path) % hosts_size_;
}

std::vector<host_t>
ForwarderDistributor::locate_directory_metadata(const std::string& path) const {
    return all_hosts_;
}
#ifdef GKFS_USE_GUIDED_DISTRIBUTION
bool
GuidedDistributor::init_guided() {
    unsigned int destination_host;
    chunkid_t chunk_id;
    string path;
    std::ifstream mapfile;
    mapfile.open(GKFS_USE_GUIDED_DISTRIBUTION_PATH);
    if((mapfile.rdstate() & std::ifstream::failbit) != 0)
        return false; // If it fails, the mapping will be as the SimpleHash

    while(mapfile >> path >> chunk_id >> destination_host) {
        // We need destination+1, as 0 has an special meaning in the interval
        // map.
        if(path.size() > 0 and path[0] == '#') {
            // Path that has this prefix will have metadata and data in the same
            // place  i.e. #/mdtest-hard/ 10 10 chunk_id and destination_host
            // are not used
            prefix_list.emplace_back(path.substr(1, path.size()));
            continue;
        }

        auto I = map_interval.find(path);
        if(I == map_interval.end())
            map_interval[path] += make_pair(
                    boost::icl::discrete_interval<chunkid_t>::right_open(
                            chunk_id, chunk_id + 1),
                    destination_host + 1);
        else if(I->second.find(chunk_id) == I->second.end())
            I->second.insert(make_pair(
                    boost::icl::discrete_interval<chunkid_t>::right_open(
                            chunk_id, chunk_id + 1),
                    destination_host + 1));
    }
    mapfile.close();
    return true;
}

GuidedDistributor::GuidedDistributor() {
    init_guided();
}

GuidedDistributor::GuidedDistributor(host_t localhost,
                                     unsigned int hosts_size) {
    if(hosts_size_ != hosts_size) {
        hosts_size_ = hosts_size;
        localhost_ = localhost;
        all_hosts_ = std::vector<unsigned int>(hosts_size);
        ::iota(all_hosts_.begin(), all_hosts_.end(), 0);
    }
    init_guided();
}

host_t
GuidedDistributor::localhost() const {
    return localhost_;
}

host_t
GuidedDistributor::locate_data(const string& path, const chunkid_t& chnk_id,
                               unsigned int hosts_size) {
    if(hosts_size_ != hosts_size) {
        hosts_size_ = hosts_size;
        all_hosts_ = std::vector<unsigned int>(hosts_size);
        ::iota(all_hosts_.begin(), all_hosts_.end(), 0);
    }

    return (locate_data(path, chnk_id));
}

host_t
GuidedDistributor::locate_data(const string& path,
                               const chunkid_t& chnk_id) const {
    auto it = map_interval.find(path);
    if(it != map_interval.end()) {
        auto it_f = it->second.find(chnk_id);
        if(it_f != it->second.end()) {
            return (it_f->second -
                    1); // Decrement destination host from the interval_map
        }
    }

    for(auto const& it : prefix_list) {
        if(0 == path.compare(0, min(it.length(), path.length()), it, 0,
                             min(it.length(), path.length()))) {
        }
        return str_hash(path) % hosts_size_;
    }

    auto locate = path + ::to_string(chnk_id);
    return str_hash(locate) % hosts_size_;
}

host_t
GuidedDistributor::locate_file_metadata(const string& path) const {
    return str_hash(path) % hosts_size_;
}

::vector<host_t>
GuidedDistributor::locate_directory_metadata(const string& path) const {
    return all_hosts_;
}
#endif
} // namespace rpc
} // namespace gkfs
