#include <AdbDevice.hpp>
#include <UsbTransport.hpp>

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "You have to provide paths to private and public keys generated by adb keygen" << std::endl;
        return 1;
    }

    const char* privateKey = argv[1];
    const char* publicKey = argv[2];


    auto usbContext = ObjLibusbContext::make();
    usbContext->spawnEventHandlingThread().detach();

    std::cout << "Searching for ADB Device..." << std::endl;
    std::unique_ptr<UsbTransport> transport;
    {
        auto vector = usbContext->getDeviceVector();
        for (const auto& device : vector) {
            auto interface = UsbTransport::findAdbInterface(device);
            if (!interface)
                continue;

            std::cout << "Device found. Read address: " << (int)interface->readEndpointAddress << ", "
            << "write address: " << (int)interface->writeEndpointAddress << std::endl;

            transport = UsbTransport::make(device);
            if (transport)
                break;
        }
    }

    if (!transport) {
        std::cout << "Couldn't create any transports" << std::endl;
        return 0;
    }
    std::cout << "Transport created." << std::endl
    << "Connecting to the device..." << std::endl;

    auto device = AdbDevice::make(std::move(transport));

    // ADD KEYS TO BE ABLE TO CONNECT TO DEVICE THAT REQUIRES AUTHORIZATION
    device->addPrivateKeyPath(privateKey);
    device->setPublicKeyPath(publicKey);

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
        << "\t  System Type: " << device->getSystemType() << std::endl
        << std::endl;

    std::cout << "Features: " << std::endl;
    if (!device->getFeatures().empty())
        for (const auto& feature : device->getFeatures())
            std::cout << "\t* " << feature << std::endl;
    else
        std::cout << " <none> " << std::endl;
    std::cout << std::endl;


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
