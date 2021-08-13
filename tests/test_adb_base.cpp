#include <iostream>
#include <thread>
#include <condition_variable>

#include <ObjLibusb/Context.hpp>

#include <AdbBase.hpp>
#include <utils.hpp>
#include <UsbTransport.hpp>

int main() {

    // Init USB:
    auto usbContext = ObjLibusbContext::make();
    auto usbThread = usbContext->spawnEventHandlingThread();
    usbThread.detach();

    // Find device:
    std::unique_ptr<Transport> transport;

    {   // Find ADB devices
        auto devices = usbContext->getDeviceVector();
        for (const auto& device : devices) {
            auto optInterface = UsbTransport::findAdbInterface(device);
            if (!optInterface)
                continue;

            std::cout << "Found ADB Device" << std::endl;

            transport = UsbTransport::makeTransport(device, *optInterface);
            break;
        }
    }

    if (!transport) {
        std::cout << "Couldn't open any ADB devices" << std::endl;
        return 1;
    }

    // Setup ADB base:
    bool received;
    uint32_t remoteStreamId = 0;
    auto base = AdbBase::makeUnique(std::move(transport));

    base->setPacketListener([&received, &remoteStreamId] (const APacket& packet) {
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

            auto hexPayload = utils::dataToHex(payload.getBuffer(), payload.getSize());
            std::cout << "hex: " << std::endl << '\t' << hexPayload << std::endl;

            std::cout << std::endl;
        }

        received = true;

        if (packet.getMessage().command == A_OKAY)
            remoteStreamId = packet.getMessage().arg0;
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
    std::string str = "shell:ls";
    base->sendOpen(10, APayload(str));
    std::cout << " -> Sent OPEN (localId = 10), destination = \"" << str << "\"" << std::endl;
    while(!received)
        std::this_thread::yield();


    if (remoteStreamId == 0)
        return 0;

    received = false;
    base->sendClose(10, remoteStreamId);
    std::cout << " -> Sent CLSE (localId = 10), remoteId = " << remoteStreamId << std::endl;
    while(!received)
        std::this_thread::yield();
}