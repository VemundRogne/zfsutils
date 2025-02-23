#include "core.hpp"
#include "testutils.hpp"

#include <exception>
#include <fcntl.h>
#include <filesystem>

int main() {
    try {
        zfs::Pool testpool_A = zfs::ZFS::getPoolByName("zfsutils_testpool_A");
        zfs::Dataset testDataset = testpool_A.openDataset("testDataset");

        testDataset.createSnapshot("initial");

        std::array<std::string, 10> snapshotNames{
            "first", "second",  "third", "fourth", "fifth",
            "sixth", "seventh", "eight", "ninth",  "tenth"};

        for (std::string &snapshotName : snapshotNames) {
            testutils::write_string_to_file(
                testDataset.getMountpoint(), snapshotName + "_testfile.txt",
                "Hello! This is some text for snapshot '" + snapshotName +
                    "'\n");
            zfs::Snapshot snap = testDataset.createSnapshot(snapshotName);
        }

        for (zfs::Snapshot &snap : testDataset.get_snapshots()) {
            std::cout << "Contents in '" + snap.name() + "':" << std::endl;

            std::string snapshot_mountpoint =
                testDataset.getMountpoint() + "/.zfs/snapshot/" + snap.name();

            // Just print out all files
            for (const auto &entry :
                 std::filesystem::directory_iterator(snapshot_mountpoint)) {
                std::cout << "  " << entry.path() << std::endl;
            }

            // And verify that snap.name() + _testfile.txt exists
            if (!std::filesystem::exists(snapshot_mountpoint + "/" +
                                         snap.name() + "_testfile.txt") &
                snap.name() != "initial") {
                throw std::logic_error{"Snapshot '" + snap.name() +
                                       "' does not have testfile '" +
                                       snap.name() + "_testfile.txt'"};
            }
        }
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    return 0;
}
