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
    if (!device->isConnected()) {
        std::cout << "Device wasn't connected..." << std::endl;
        return 0;
    }
    std::cout << "Device connected" << std::endl;

    std::string dest = "shell:echo \"Hello, world!\" ";
    std::cout << "open stream to " << dest << std::endl;
    auto streams = device->open(dest);
    if (!streams) {
        std::cout << "Streams did not open" << std::endl;
        return 0;
    }

    std::cout << "Streams opened" << std::endl;
    auto& in = streams->istream;

    std::string str;
    in >> str;
    std::cout << " > " << str << std::endl;

    return 0;
}
