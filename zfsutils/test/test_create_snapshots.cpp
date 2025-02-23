#include "core.hpp"
#include "libzfs.h"
#include <exception>
#include <filesystem>
#include <fstream>

#include <fcntl.h>
#include <stdexcept>
#include <thread>

std::vector<char> data;

void processPipeThread(int readFd) {
    ssize_t bytesRead;
    char buffer[4096];

    // We just read until we get something else than data
    while ((bytesRead = read(readFd, buffer, sizeof(buffer))) > 0) {
        std::cout << "Got some data..." << bytesRead << std::endl;
        for (int i = 0; i < bytesRead; i++) {
            data.push_back(buffer[i]);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    try {
        zfs::Pool testpool_A = zfs::ZFS::getPoolByName("zfsutils_testpool_A");
        zfs::Dataset testDataset = testpool_A.openDataset("testDataset");

        // Now we want to make a snapshot _before_ doing anything on the
        // dataset. Then we want to 'put' something in the dataset, and then
        // make another snapshot

        zfs::Snapshot firstSnapshot =
            testDataset.createSnapshot("initialSnapshot");

        try {
            std::ofstream outFile;
            outFile.exceptions(std::ofstream::badbit | std::ofstream::failbit);
            outFile.open(testDataset.getMountpoint() + "/testfile.txt");
            outFile << "Hello, there! This is some text!" << std::endl;
            outFile.close();

        } catch (const std::ofstream::failure &e) {
            std::cerr << e.what() << std::endl;
            return -1;
        }

        if (!std::filesystem::exists(testDataset.getMountpoint() +
                                     "/testfile.txt")) {
            std::cerr << "Did not manage to make the testfile :(" << std::endl;
            return -1;
        }

        std::string secondSnapshotName = "secondSnapshot";
        zfs::Snapshot secondSnapshot =
            testDataset.createSnapshot(secondSnapshotName);

        if (secondSnapshot.name() != secondSnapshotName) {
            std::cout << "'" + secondSnapshot.name() + "'"
                      << "!=" << "'" + secondSnapshotName + "'" << std::endl;
            return -1;
        }

        // Assert that the file _is not_ in the firstSnapshot
        if (std::filesystem::exists(testDataset.getMountpoint() +
                                    "/.zfs/snapshot/" + firstSnapshot.name() +
                                    "/testfile.txt")) {
            std::cerr << "File is in snapshot where it should not" << std::endl;
            return -1;
        }

        // Assert that the file _is_ in the second snapshot
        if (!std::filesystem::exists(testDataset.getMountpoint() +
                                     "/.zfs/snapshot/" + secondSnapshot.name() +
                                     "/testfile.txt")) {
            std::cerr << "File is _not_ in snapshot where it should"
                      << std::endl;
            return -1;
        }

        // Unmount the dataset
        zfs_unmount(testDataset.getHandle(), nullptr, 0);

        // Assert that the file is no longer accessible
        if (std::filesystem::exists(testDataset.getMountpoint() +
                                    "/.zfs/snapshot/" + secondSnapshot.name() +
                                    "/testfile.txt")) {
            std::cerr << "File is somehow accessible, when I tried to unmount "
                         "the dataset:("
                      << std::endl;
            return -1;
        }

        // Now can we send the initial snapshot to pool B?!
        // But before that! We just send the data out of stdout...
        zfs::Pool testpool_B = zfs::ZFS::getPoolByName("zfsutils_testpool_B");

        int mypipe[2];
        int pipe_retval = pipe(mypipe);
        if (pipe_retval != 0) {
            throw std::logic_error("Pipe open fail...");
        }

        std::thread processorThread{processPipeThread, mypipe[0]};

        sendflags_t flags = {0};
        flags.replicate = B_TRUE;
        int zfs_send_retval =
            zfs_send(testDataset.getHandle(), NULL, "secondSnapshot", &flags,
                     mypipe[1], NULL, NULL, NULL);

        // We have to close the pipe -- because the processorThread needs to
        // know when there is no more data
        close(mypipe[1]);

        processorThread.join();
        close(mypipe[0]);

        std::cout << "ZFS send retval: " << zfs_send_retval << std::endl;
        std::cout << "Amount of data " << data.size() << std::endl;

        /*
        std::string send_filename = "zfs_send_output.zfs";
        int fileFd =
            open(send_filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fileFd == -1) {
            throw std::logic_error("file fail");
        }

        sendflags_t flags = {0};
        flags.replicate = B_TRUE;
        int zfs_send_retval =
            zfs_send(testDataset.getHandle(), NULL, "secondSnapshot", &flags,
                     fileFd, NULL, NULL, NULL);
        close(fileFd);

        std::cout << "ZFS send retval: " << zfs_send_retval << std::endl;

        // reception:
        int recv_fileFd = open(send_filename.c_str(), O_RDONLY);
        if (fileFd == -1) {
            throw std::logic_error("file fail");
        }
        zfs::ZFSHandle &zfsHandle = zfs::ZFSHandle::instance();
        recvflags_t recvflags;
        recvflags.force = B_TRUE;
        int zfs_recv_retval =
            zfs_receive(zfsHandle.get(), testpool_B.name().c_str(), NULL,
                        &recvflags, recv_fileFd, NULL);

        close(recv_fileFd);

        std::cout << "ZFS receive retval: " << zfs_recv_retval << std::endl;
        */

    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    return 0;
}
