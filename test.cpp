#include "libzfs.h"
#include "sys/nvpair.h"
#include "sys/stdtypes.h"

#include <exception>
#include <iostream>

#include <chrono>
#include <iomanip>
#include <sstream>

#include "core.hpp"

#include <thread>

void print_nvlist(zfs_handle_t *zh) {
    // Expects zh to be an initialized handle...
    nvlist_t *props = zfs_get_all_props(zh);

    nvpair_t *pair = nullptr;

    while ((pair = nvlist_next_nvpair(props, pair)) != nullptr) {
        const char *name = nvpair_name(pair);
        std::cout << "Name: " << name << std::endl << "  Type: ";

        data_type_t type = nvpair_type(pair);

        switch (type) {
        case DATA_TYPE_STRING:
            std::cout << "String!" << std::endl;
            break;
        case DATA_TYPE_STRING_ARRAY:
            std::cout << "String-array!" << std::endl;
            break;
        default:
            std::cout << "not handled..." << std::endl;
        }
    }
}

int print_dataset_name(zfs_handle_t *zh, void *data) {
    std::cout << " - " << zfs_get_name(zh) << std::endl;
    zfs_iter_filesystems(zh, print_dataset_name, nullptr);
    return 0;
}

bool create_snapshot(libzfs_handle_t *g_zfs, const std::string &dataset_name,
                     const std::string &snapshot_name) {
    std::string full_snapshot_name = dataset_name + "@" + snapshot_name;

    int ret = zfs_snapshot(g_zfs, full_snapshot_name.c_str(), B_FALSE, nullptr);
    if (ret != 0) {
        std::cerr << "Failed to create snapshot: " << full_snapshot_name
                  << " - " << libzfs_error_description(g_zfs) << std::endl;
        return false;
    }

    std::cout << "Snapshot created: " << full_snapshot_name << std::endl;
    return true;
}

// Return Value Optimization?
zfs::Pool some_func() {
    zfs::ZFSHandle &zfsHandle = zfs::ZFSHandle::instance();
    zfs::Pool myPool = zfsHandle.getPoolByName("pool2");
    std::cout << "Pool pointer (in func): " << myPool.handle_ << std::endl;
    std::cout << "Got pool: " << myPool.name() << std::endl;
    return std::move(myPool);
}

int main() {
    // Get the singleton
    zfs::ZFSHandle &zfsHandle = zfs::ZFSHandle::instance();

    zfs::Pool pool2FromFunc = some_func();
    zfs::Pool pool2Again = zfsHandle.getPoolByName("pool2");

    std::cout << "Pool pointer 1: " << pool2FromFunc.handle_ << std::endl;
    std::cout << "Pool pointer 2: " << pool2Again.handle_ << std::endl;

    const char *state1 = zpool_get_state_str(pool2FromFunc.handle_);
    const char *state2 = zpool_get_state_str(pool2Again.handle_);

    std::cout << "State 1 " << state1 << std::endl;
    std::cout << "State 2 " << state2 << std::endl;

    try {
        zfs::Pool yetAnotherPool{0};
        yetAnotherPool = std::move(pool2FromFunc);
        std::cout << "Pool name: " << yetAnotherPool.name() << std::endl;
        std::cout << "Pool name: " << pool2FromFunc.name() << std::endl;
    } catch (std::exception e) {
        std::cout << "Caught exception!" << std::endl;
    }

    // zfs::Pool myPool = zfsHandle.getPoolByName("pool2");
    // std::cout << "Got pool: " << myPool.name() << std::endl;

    // zfsHandle.initPoolHandles();
    //   Iterate over pools and get their names. This is probably bad since it
    //   uses the copy-operator
    //  std::cout << "Pools:" << std::endl;
    //  for (zfs::Pool pool : zfsHandle.pools) {
    //  std::cout << " " << pool.name() << std::endl;
    // }

    // for (int i = 0; i < 100; i++) {
    // zfs::Pool pool = zfsHandle.pools[0];
    // std::cout << "Pool name: " << pool.name() << std::endl;
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    //}

    // zfs_handle_t *zh = zfs_open(zfsHandle.get(), "pool1",
    // ZFS_TYPE_FILESYSTEM); if (!zh) { std::cerr << "Failed to open zfs pool"
    // << std::endl; return 1;
    //}

    // print_nvlist(zh);

    // zfs_iter_filesystems(zh, print_dataset_name, nullptr);

    // std::string dataset_name = "pool1";

    // auto now = std::chrono::system_clock::now();
    // std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    // std::tm now_tm = *std::gmtime(&now_time_t);
    // std::ostringstream oss;
    // oss << std::put_time(&now_tm, "%Y-%m-%dT%H:%M");

    // std::string snapshot_name = oss.str();

    // std::cout << "Snapshot name: " << "'" << snapshot_name << "'" <<
    // std::endl; create_snapshot(g_zfs, dataset_name, snapshot_name);

    return 0;
}
