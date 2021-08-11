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

    // ADD KEYS TO BE ABLE TO CONNECT TO DEVICE THAT REQUIRES AUTHORIZATION
    device->addPrivateKeyPath("/home/user/.android/adbkey");
    device->setPublicKeyPath("/home/user/.android/adbkey.pub");

    // CONNECT TO THE DEVICE
    device->connect();
    if (!device->isConnected()) {
        std::cout << "Couldn't establish connection with the device." << std::endl;
        return 0;
    }
    std::cout << "Device connected." << std::endl;
    std::cout << "Info:"
        << " Product: " << device->getProduct() << std::endl
        << "\t  Model: " << device->getModel() << std::endl
        << "\t  Device: " << device->getDevice() << std::endl
        << "\t  Serial: " << device->getSerial() << std::endl
        << std::endl;


    std::string dest = "shell:echo \"Hello, world!\"";
    std::cout << "Opening stream to " << dest << "..." << std::endl;

    // OPEN STREAM TO A CHOSEN DESTINATION
    auto streams = device->open(dest);
    if (!streams) {
        std::cout << "Couldn't open the streams." << std::endl;
        return 0;
    }

    std::cout << "The streams opened." << std::endl;
    auto& in = streams->istream;

    // READ FROM THE STREAM
    std::string result;
    in >> result;
    std::cout << "The input stream returned: \"" << result << '"' << std::endl;

    return 0;
}
