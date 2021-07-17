#include <iostream>
#include <vector>
#include <utility>
#include <libadb.hpp>

#include <LibusbContext.hpp>
#include <LibusbDevice.hpp>
#include <LibusbDeviceList.hpp>
#include <LibusbDeviceHandle.hpp>


int main()
{
    auto context = LibusbContext::makeContext();
    std::vector<std::pair<LibusbDevice, interface_data>> adbDevices;

    {   // list block
        auto list = context->getDeviceVector();
        if (list.empty()) {
            std::cerr << "Cannot get device list" << std::endl;
            return 1;
        }

        for (auto& device : list) {
            std::optional<interface_data> optInterface = find_adb_interface(device);
            if (optInterface.has_value())
                adbDevices.emplace_back(std::move(device), optInterface.value());
        }

    }   // end of list block

    std::cout << "ADB Devices count: " << adbDevices.size() << std::endl;
    for (size_t i = 0; i < adbDevices.size(); ++i) {
        auto deviceDescriptor = adbDevices[i].first.getDescriptor();
        std::cout << i + 1 << ". VenID: " << deviceDescriptor->idVendor << ", ProdID: " << deviceDescriptor->idProduct
                  << std::endl;
    }

    if (adbDevices.empty())
        return 0;

    auto& device = adbDevices[0].first;
    auto& interface = adbDevices[0].second;

    auto deviceHandle = device.open();

    auto thread = context->spawnEventHandlingThread();
    thread.detach();



    

    context.reset();
    return 0;
}
