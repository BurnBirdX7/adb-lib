#include <LibusbContext.hpp>
#include <LibusbDevice.hpp>

#include <AdbDevice.hpp>
#include <UsbTransport.hpp>

#include <iostream>

int main() {
    auto usbContext = LibusbContext::makeContext();
    usbContext->spawnEventHandlingThread().detach();

    std::unique_ptr<UsbTransport> transport;
    {
        auto vector = usbContext->getDeviceVector();
        for (const auto& device : vector) {
            transport = UsbTransport::makeTransport(device);
            if (transport)
                break;
        }
    }

    if (!transport) {
        std::cout << "Couldn't create any transports" << std::endl;
        return 0;
    }

    auto device = AdbDevice::make(std::move(transport));

    device->connect();
    if (device->isConnected())
        std::cout << "Device connected" << std::endl;
    else {
        std::cout << "Device wasn't connected..." << std::endl;
        return 0;
    }

    auto streams = device->open("shell");
    if (streams)
        std::cout << "Streams were opened" << std::endl;
    else
        std::cout << "Streams were not opened" << std::endl;

    return 0;
}
