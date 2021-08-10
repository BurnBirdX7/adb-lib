#include <LibusbContext.hpp>
#include <LibusbDevice.hpp>

#include <AdbDevice.hpp>
#include <UsbTransport.hpp>

#include <iostream>

int main() {
    auto usbContext = LibusbContext::makeContext();
    usbContext->spawnEventHandlingThread().detach();

    std::cout << "Searching for ADB Device..." << std::endl;
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
    std::cout << "Device found. Transport created." << std::endl
    << "Connecting to the device..." << std::endl;

    auto device = AdbDevice::make(std::move(transport));
    device->connect();  // CONNECT TO THE DEVICE
    if (!device->isConnected()) {
        std::cout << "Couldn't establish connection with device." << std::endl;
        return 0;
    }
    std::cout << "Device connected." << std::endl;

    std::string dest = "shell:echo \"Hello, world!\"";
    std::cout << "Opening stream to " << dest << "..." << std::endl;
    auto streams = device->open(dest);  // OPEN STREAM
    if (!streams) {
        std::cout << "Couldn't open the streams." << std::endl;
        return 0;
    }

    std::cout << "The streams opened." << std::endl;
    auto& in = streams->istream;

    std::string result;
    in >> result;  // READ FROM STREAM
    std::cout << "The input stream returned: \"" << result << '"' << std::endl;

    return 0;
}
