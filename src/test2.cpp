#include <iostream>
#include <thread>

#include <LibusbContext.hpp>
#include <LibusbDevice.hpp>

#include <AdbBase.hpp>

#include <UsbTransport.hpp>

int main() {

    auto usbContext = LibusbContext::makeContext();
    auto usbThread = usbContext->spawnEventHandlingThread();
    usbThread.detach();

    auto devices = usbContext->getDeviceVector();

    std::unique_ptr<Transport> transport;

    for (const auto& device : devices) {
        auto optInterface = UsbTransport::findAdbInterface(device);
        if (!optInterface)
            continue;

        std::cout << "Found ADB Device" << std::endl;

        transport = UsbTransport::makeTransport(device, *optInterface);
        break;
    }

    if (!transport) {
        std::cout << "Couldn't open any ADB devices" << std::endl;
        return 1;
    }

    auto base = AdbBase::makeUnique(std::move(transport));

    base->setPacketListener([] (const APacket& packet) {
        std::cout << "Head: ";
        for(size_t i = 0; i < 4; ++i)
            std::cout << reinterpret_cast<unsigned char*>(packet.getMessage().command)[i];
        std::cout << std::endl;

        std::cout << "APayload (size: " << packet.getMessage().dataLength << "): "<< std::endl;
        if (packet.hasPayload()) {
            auto size = packet.getPayload().getSize();
            for(size_t i = 0; i < size; ++i)
                std::cout << packet.getPayload()[i];
            std::cout << std::endl;
        }
    });

    base->setErrorListener([] (int errorCode, const APacket* packet, bool incomingPacket) {

        std::cerr << "Error: " << errorCode << std::endl
                    << "Incoming: " << std::boolalpha << incomingPacket << std::endl;

    });

    base->sendConnect(Features::getFullSet());

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));


}