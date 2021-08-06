#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <LibusbContext.hpp>
#include <LibusbDevice.hpp>

#include <AdbBase.hpp>
#include <utils.hpp>
#include <UsbTransport.hpp>

int main() {

    // Init USB:
    auto usbContext = LibusbContext::makeContext();
    auto usbThread = usbContext->spawnEventHandlingThread();
    usbThread.detach();

    // Find device:
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

    devices.resize(0); // unref devices

    if (!transport) {
        std::cout << "Couldn't open any ADB devices" << std::endl;
        return 1;
    }

    // Setup ADB base:
    bool received;
    auto base = AdbBase::makeUnique(std::move(transport));

    base->setPacketListener([&received] (const APacket& packet) {
        const auto& msg = packet.getMessage();
        std::cout << "Head: " << std::endl
            << "\tcmd: " << msg.viewCommand() << std::endl
            << "\targ0: " << msg.arg0 << " (hex: " << std::hex << msg.arg0 << std::dec << ")" << std::endl
            << "\targ1: " << msg.arg1 << std::endl
            << std::endl;

        std::cout << "APayload (size: " << packet.getMessage().dataLength << ") "<< std::endl;
        if (packet.hasPayload() && packet.getPayload().getSize() > 0) {
            const auto& payload = packet.getPayload();
            std::cout << "ascii: " << std::endl << '\t';
            for(const auto& ch : payload)
                if (ch == 0)
                    std::cout << "{0}";
                else
                    std::cout << ch;
            std::cout << std::endl;

            auto hexPayload = utils::dataToHex(payload.toStringView());
            std::cout << "hex: " << std::endl << '\t' << hexPayload << std::endl;

            std::cout << std::endl;
        }

        received = true;
    });

    base->setErrorListener([] (int errorCode, const APacket* packet, bool incomingPacket) {
        std::cerr << "Error Code: " << errorCode << std::endl
                    << "Incoming: " << std::boolalpha << incomingPacket << std::endl;

    });


    // Communicate:
    received = false;
    base->sendConnect("host", Features::getFullSet());
    std::cout << " -> Sent CNXN" << std::endl;
    while(!received)
        std::this_thread::yield();

    received = false;
    base->sendOpen(10, APayload("shell"));
    std::cout << " -> Sent OPEN (localId = 10), destination = \"shell\"" << std::endl;
    while(!received)
        std::this_thread::yield();
}