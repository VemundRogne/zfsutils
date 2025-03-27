#include "core.hpp"

#include <exception>
#include <filesystem>
#include <thread>

#include "piper.hpp"
#include "serial.hpp"

std::vector<char> data;

void pipe_to_data(SerialReader &reader) {
    while (auto received = reader.get(5000)) {
        std::cout << "Got " << received.value().size() << " bytes of data"
                  << std::endl;

        for (auto &c : received.value()) {
            data.push_back(c);
        }
    }
}

void pipe_from_data(SerialWriter &writer) {
    for (char &datapoint : data) {
        int retval = writer.send(datapoint);
        if (retval != 1) {
            throw std::logic_error{"pipe from data failed to write a byte"};
        }
    }
    writer.terminate();
}

int main() {
    try {
        zfs::Pool testpool_A = zfs::ZFS::getPoolByName("zfsutils_testpool_A");
        zfs::Dataset sourceDataset = testpool_A.openDataset("testDataset");

        zfs_unmount(sourceDataset.getHandle(), nullptr, 0);

        // Now can we send the initial snapshot to pool B?!
        // But before that! We just send the data out of stdout...
        zfs::Pool testpool_B = zfs::ZFS::getPoolByName("zfsutils_testpool_B");

        {
            Piper piper;
            RxPipe rxPipe = piper.getRx();
            TxPipe txPipe = piper.getTx();

            std::thread processorThread{pipe_to_data, std::ref(rxPipe)};

            sendflags_t flags = {0};
            flags.replicate = B_TRUE;
            // flags.doall = B_TRUE;
            int zfs_send_retval =
                zfs_send(sourceDataset.getHandle(), NULL, "third", &flags,
                         txPipe.getPipeFd(), NULL, NULL, NULL);

            // We have to close the pipe -- because the processorThread needs to
            // know when there is no more data
            txPipe.terminate();

            processorThread.join();

            std::cout << "ZFS send retval: " << zfs_send_retval << std::endl;
            std::cout << "Amount of data " << data.size() << std::endl;
        }

        {
            Piper piper;
            RxPipe rxPipe = piper.getRx();
            TxPipe txPipe = piper.getTx();

            std::thread data_to_pipe_thread{pipe_from_data, std::ref(txPipe)};

            zfs::ZFSHandle &zfsHandle = zfs::ZFSHandle::instance();

            std::cout << testpool_B.name() << std::endl;

            recvflags myRecvflags{};
            // myRecvflags.force = B_TRUE;
            myRecvflags.verbose = B_TRUE;
            myRecvflags.nomount = B_TRUE;
            int zfs_recv_retval = zfs_receive(
                zfsHandle.get(), (testpool_B.name() + "/testDataset").c_str(),
                NULL, &myRecvflags, rxPipe.getPipeFd(), NULL);
            data_to_pipe_thread.join();

            std::cout << "zfs receive retval: " << zfs_recv_retval << std::endl;
        }

        zfs::Dataset dataset_in_b = testpool_B.openDataset("testDataset");

        zfs_mount(dataset_in_b.getHandle(), nullptr, 0);

        for (zfs::Snapshot &snap : dataset_in_b.get_snapshots()) {
            std::cout << "Contents in '" + snap.name() + "':" << std::endl;

            std::string snapshot_mountpoint =
                dataset_in_b.getMountpoint() + "/.zfs/snapshot/" + snap.name();

            // Just print out all files
            for (const auto &entry :
                 std::filesystem::directory_iterator(snapshot_mountpoint)) {
                std::cout << "  " << entry.path() << std::endl;
            }
        }

        // Assert that a snapshot called "third" is present
        try {
            zfs::Snapshot snap = dataset_in_b.openSnapshot("third");
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
            return -1;
        }

    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
    return 0;
}
