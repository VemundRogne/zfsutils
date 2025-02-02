#include "core.hpp"
#include "libzfs.h"
#include <exception>
#include <filesystem>
#include <fstream>

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
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    return 0;
}
