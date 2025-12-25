#include "zfsutils/dataset.hpp"
#include "zfsutils/pool.hpp"

#include <iostream>
#include <ranges>

class MockSnapshot : public interface::ISnapshot {
  public:
    std::string name() const override { return name_; };
    MockSnapshot(std::string name) : name_{name} {
        std::cout << "MockSnapshot " << name_ << " at " << this
                  << " constructed!" << std::endl;
    };
    ~MockSnapshot() {
        std::cout << "MockSnapshot " << name_ << " at " << this << " destroyed!"
                  << std::endl;
    }
    MockSnapshot() = delete;

    MockSnapshot(MockSnapshot &&other) noexcept
        : name_{std::move(other.name_)} {
        other.name_ = name_ + "_old";
        std::cout << "MockSnapshot " << name_ << " at " << this
                  << " move-constructed from " << &other << std::endl;
    }

  private:
    std::string name_;
};

class MockDataset : public interface::IDataset {
  public:
    std::string name() const override { return name_; };

    std::vector<std::shared_ptr<interface::ISnapshot>> getVirtSnap() override {
        std::vector<std::shared_ptr<interface::ISnapshot>> snaps;

        for (auto &sharedSnap : sharedMockedSnapshots) {
            snaps.push_back(
                std::static_pointer_cast<interface::ISnapshot>(sharedSnap));
        }

        return snaps;
    };

    MockDataset(std::string name) : name_{name} {
        std::cout << "MockDataset " << name_ << " constructed!" << std::endl;
    };

    MockDataset(std::string name, std::vector<MockSnapshot> &mockedSnapshots)
        : name_{name} {
        std::cout << "MockDataset " << name_ << " constructed!" << std::endl;
        for (auto &snap : mockedSnapshots) {
            std::cout << "Making shared and pushing " << &snap << std::endl;
            sharedMockedSnapshots.push_back(
                std::make_shared<MockSnapshot>(std::move(snap)));
        }
    }

    // Using this variant is more performant (less moves/copies)
    MockDataset(std::string name,
                std::vector<std::shared_ptr<MockSnapshot>> &mockedSnaps)
        : name_{name} {
        for (auto &snap : mockedSnaps) {
            sharedMockedSnapshots.push_back(snap);
        }
    }
    ~MockDataset() {
        std::cout << "MockDataset " << name_ << " destroyed!" << std::endl;
    }

  private:
    std::string name_;
    std::vector<std::shared_ptr<MockSnapshot>> sharedMockedSnapshots{};
};

std::string recent_common_snap(zfsutils::Dataset &source,
                               zfsutils::Dataset &destination) {
    std::vector<zfsutils::Snapshot> source_snapshots = source.getSnapshots();

    std::vector<zfsutils::Snapshot> destination_snapshots =
        destination.getSnapshots();

    int n_comp{0};

    // This should not really be a for-loop...
    //
    // If the latest snap in destionation is not present in src then we have a
    // problem anyway, and finding a previous shared state is only for fixing
    // that error-state
    for (auto &dest_snap : std::ranges::views::reverse(destination_snapshots)) {
        for (auto &src_snap : std::ranges::views::reverse(source_snapshots)) {
            n_comp++;
            if (dest_snap.name() == src_snap.name()) {
                std::cout << "COMMON SNAP: " << dest_snap.name() << std::endl;
                std::cout << "  found in " << n_comp << " comparisons"
                          << std::endl;
                return "";
            }
        }
    }
    return "";
}

void compare(zfsutils::Dataset source, zfsutils::Dataset destination) {
    std::cout << "Source dataset '" << source.name()
              << "' has snaps: " << std::endl;
    for (auto &snap : source.getSnapshots()) {
        std::cout << "  " << snap.name() << std::endl;
    }

    std::cout << "Destination dataset '" << destination.name()
              << "'has snaps: " << std::endl;
    for (auto &snap : destination.getSnapshots()) {
        std::cout << "  " << snap.name() << std::endl;
    }

    // We need to find the most recent snapshot that is present on both devices
    // One might assume that this first call is faster than the second (and they
    // should give the same result)
    recent_common_snap(source, destination);
    recent_common_snap(destination, source);
}

void useDataset(interface::IDataset &dataset) {
    std::cout << " --- USE DATASET --- " << std::endl;
    std::cout << " IDataset " << dataset.name() << std::endl;
    for (auto &snap : dataset.getVirtSnap()) {
        std::cout << "   ISnapshot " << snap->name() << std::endl;
    }
    std::cout << " --- USE DATASET END --- " << std::endl;
}

MockDataset getDataset() {
    std::vector<std::shared_ptr<MockSnapshot>> mockedSnaps;
    mockedSnaps.emplace_back(std::make_shared<MockSnapshot>("data1@snap1"));
    mockedSnaps.emplace_back(std::make_shared<MockSnapshot>("data1@snap2"));
    mockedSnaps.emplace_back(std::make_shared<MockSnapshot>("data1@snap3"));
    mockedSnaps.emplace_back(std::make_shared<MockSnapshot>("data1@snap4"));

    std::cout << "Snap in main are " << &mockedSnaps[0] << " and "
              << &mockedSnaps[1] << std::endl;

    MockDataset data1{"data1", mockedSnaps};
    return data1;
}

int main() {
    /*
    auto testpoolA = zfsutils::Pool::open("zfsutils_testpool_A");
    auto testpoolB = zfsutils::Pool::open("zfsutils_testpool_B");

    auto datasetA = testpoolA.openDataset("testDataset");
    auto datasetB = testpoolB.openDataset("testDataset");
    if (!datasetA.has_value() || !datasetB.has_value()) {
        throw std::runtime_error{"Could not open datasets..."};
    }
    */

    // compare(std::move(datasetA.value()), std::move(datasetB.value()));

    MockDataset data1 = getDataset();
    useDataset(data1);
    // MockDataset data2{
    //"data1", {MockSnapshot{"data2/snap1"}, MockSnapshot{"data2/snap2"}}};

    return 0;
}
