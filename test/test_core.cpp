#include <iostream>
#include <memory>

#include "zfsutils/core.hpp"

typedef struct {
    int id;
} someHandle_t;

void closer(someHandle_t *handle) {
    std::cout << "Closing handle with ID " << handle->id << std::endl;
    handle->id = 0;
}

class HandleHelperTester
    : private zfsutils::internal::HandleHelper<someHandle_t, closer> {
    using Base = zfsutils::internal::HandleHelper<someHandle_t, closer>;

  public:
    HandleHelperTester(someHandle_t *handle) : Base{handle} {}
    using Base::getHandle;
    using Base::hasHandle;

    void DoSomething() { std::cout << getHandle() << std::endl; }
};

void user(HandleHelperTester myHandle) {
    myHandle.DoSomething();
    std::cout << "Handle (through escape hatch) " << myHandle.getHandle()->id
              << std::endl;
}

int main() {
    someHandle_t handle;
    handle.id = 3;

    auto myHandle = HandleHelperTester{&handle};

    myHandle.DoSomething();

    user(std::move(myHandle));

    assert(myHandle.hasHandle() == false);
}
