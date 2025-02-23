#include "core.hpp"

#include <exception>
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

    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
    return 0;
}
