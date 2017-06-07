//
// Created by evie on 6/6/17.
//

#include "db_ops.hpp"


using namespace rocksdb;
using namespace std;

inline const string db_get_mdata_helper(const string& key) {
    auto db = ADAFS_DATA->rdb().get();
    string val_str;
    db->Get(ReadOptions(), key, &val_str);
    return val_str;
}

template<>
unsigned long db_get_mdata<unsigned long>(const string& key) {
    return stoul(db_get_mdata_helper(key));
}

template<>
long db_get_mdata<long>(const string& key) {
    return stol(db_get_mdata_helper(key));
}

template<>
unsigned int db_get_mdata<unsigned int>(const string& key) {
    return static_cast<unsigned int>(stoul(db_get_mdata_helper(key)));
}

bool db_dentry_exists(const fuse_ino_t p_inode, const string& name, string& val) {
    auto db = ADAFS_DATA->rdb().get();
    auto key = fmt::FormatInt(p_inode).str() + "_d_" + name;
    return db->Get(rocksdb::ReadOptions(), key, &val).ok();
}

bool db_mdata_exists(const string& key) {
    auto db = ADAFS_DATA->rdb().get();
    string val_str;
    return db->Get(ReadOptions(), key, &val_str).ok();
}

bool db_put_dentry(const string& key, const string& val) {
    auto db = ADAFS_DATA->rdb().get();
    return db->Put(rocksdb::WriteOptions(), key, val).ok();
}

void db_get_dentries(vector<Dentry>& dentries, const fuse_ino_t dir_inode) {
    string key;
    string val;
    size_t pos;
    auto delim = "_"s;
    auto db = ADAFS_DATA->rdb();
    auto prefix = fmt::FormatInt(dir_inode).str() + "_d"s;
    // Do RangeScan on parent inode
    auto dentry_iter = db->NewIterator(rocksdb::ReadOptions());
    for (dentry_iter->Seek(prefix);
         dentry_iter->Valid() && dentry_iter->key().starts_with(prefix); dentry_iter->Next()) {
        key = dentry_iter->key().ToString();
        val = dentry_iter->value().ToString();

        // Retrieve filename from key
        pos = key.find(delim); // Split <ParentInode_d_filename> by _
        key.erase(0, pos + 3); // Erase ParentInode + _d_
        Dentry dentry{key}; // key holds only filename

        // Retrieve inode and mode from val
        pos = val.find(delim); // Split <inode_mode> by _
        dentry.inode(static_cast<fuse_ino_t>(stoul(val.substr(0, pos)))); // Substring from 0 to pos holds inode
        val.erase(0, pos + 1); // Erase inode + delim
        dentry.mode(static_cast<mode_t>(stoul(val))); // val holds only mode
        // append dentry to dentries vector
        ADAFS_DATA->spdlogger()->info("Retrieved dentry: name {} inode {} mode {}", dentry.name(), dentry.inode(),
                                      dentry.mode());
        dentries.push_back(dentry);
    }
}



