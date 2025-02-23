#include "core.hpp"

#include <exception>
#include <filesystem>
#include <thread>

std::vector<char> data;

void pipe_to_data(int readFd) {
    ssize_t bytesRead;
    char buffer[4096];

    // We just read until we get something else than data
    while ((bytesRead = read(readFd, buffer, sizeof(buffer))) > 0) {
        std::cout << "Got some data..." << bytesRead << std::endl;
        for (int i = 0; i < bytesRead; i++) {
            data.push_back(buffer[i]);
        }
        // std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void pipe_from_data(int writeFd) {
    for (char &datapoint : data) {
        int retval = write(writeFd, &datapoint, 1);
        if (retval != 1) {
            throw std::logic_error{"pipe from data failed to write a byte"};
        }
    }
    close(writeFd);
}

int main() {
    try {
        zfs::Pool testpool_A = zfs::ZFS::getPoolByName("zfsutils_testpool_A");
        zfs::Dataset sourceDataset = testpool_A.openDataset("testDataset");

        zfs_unmount(sourceDataset.getHandle(), nullptr, 0);

        // Now can we send the initial snapshot to pool B?!
        // But before that! We just send the data out of stdout...
        zfs::Pool testpool_B = zfs::ZFS::getPoolByName("zfsutils_testpool_B");

        int mypipe[2];
        int pipe_retval = pipe(mypipe);
        if (pipe_retval != 0) {
            throw std::logic_error("Pipe open fail...");
        }

        std::thread processorThread{pipe_to_data, mypipe[0]};

        sendflags_t flags = {0};
        flags.replicate = B_TRUE;
        // flags.doall = B_TRUE;
        int zfs_send_retval = zfs_send(sourceDataset.getHandle(), NULL, "third",
                                       &flags, mypipe[1], NULL, NULL, NULL);

        // We have to close the pipe -- because the processorThread needs to
        // know when there is no more data
        close(mypipe[1]);

        processorThread.join();
        close(mypipe[0]);

        std::cout << "ZFS send retval: " << zfs_send_retval << std::endl;
        std::cout << "Amount of data " << data.size() << std::endl;

        int recvPipe[2];
        int recvPipeRetval = pipe(mypipe);
        if (recvPipeRetval != 0) {
            throw std::logic_error("recvPipe open fail...");
        }

        std::thread data_to_pipe_thread{pipe_from_data, mypipe[1]};

        zfs::ZFSHandle &zfsHandle = zfs::ZFSHandle::instance();

        std::cout << testpool_B.name() << std::endl;

        recvflags myRecvflags{};
        // myRecvflags.force = B_TRUE;
        myRecvflags.verbose = B_TRUE;
        myRecvflags.nomount = B_TRUE;
        int zfs_recv_retval = zfs_receive(
            zfsHandle.get(), (testpool_B.name() + "/testDataset").c_str(), NULL,
            &myRecvflags, mypipe[0], NULL);
        data_to_pipe_thread.join();

        std::cout << "zfs receive retval: " << zfs_recv_retval << std::endl;

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

    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
    return 0;
}
