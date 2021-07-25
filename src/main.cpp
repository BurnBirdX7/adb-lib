#include <iostream>
#include <vector>
#include <mutex>
#include <memory>
#include <condition_variable>


#include <LibusbContext.hpp>
#include <LibusbDeviceList.hpp>
#include <LibusbError.hpp>

#include <UsbTransport.hpp>

// FROM adb/transport.cpp
const char* const kFeatureShell2 = "shell_v2";
const char* const kFeatureCmd = "cmd";
const char* const kFeatureStat2 = "stat_v2";
const char* const kFeatureLs2 = "ls_v2";
const char* const kFeatureLibusb = "libusb";
const char* const kFeaturePushSync = "push_sync";
const char* const kFeatureApex = "apex";
const char* const kFeatureFixedPushMkdir = "fixed_push_mkdir";
const char* const kFeatureAbb = "abb";
const char* const kFeatureFixedPushSymlinkTimestamp = "fixed_push_symlink_timestamp";
const char* const kFeatureAbbExec = "abb_exec";
const char* const kFeatureRemountShell = "remount_shell";
const char* const kFeatureTrackApp = "track_app";
const char* const kFeatureSendRecv2 = "sendrecv_v2";
const char* const kFeatureSendRecv2Brotli = "sendrecv_v2_brotli";
const char* const kFeatureSendRecv2LZ4 = "sendrecv_v2_lz4";
const char* const kFeatureSendRecv2Zstd = "sendrecv_v2_zstd";
const char* const kFeatureSendRecv2DryRunSend = "sendrecv_v2_dry_run_send";
const char* const kFeatureOpenscreenMdns = "openscreen_mdns";
// !

using FeatureSet = std::vector<std::string>;
static const FeatureSet& getFeatureSet()
{
    const static FeatureSet set = {
            kFeatureShell2,
            kFeatureCmd,
            kFeatureStat2,
            kFeatureLs2,
            kFeatureFixedPushMkdir,
            kFeatureApex,
            kFeatureAbb,
            kFeatureFixedPushSymlinkTimestamp,
            kFeatureAbbExec,
            kFeatureRemountShell,
            kFeatureTrackApp,
            kFeatureSendRecv2,
            kFeatureSendRecv2Brotli,
            kFeatureSendRecv2LZ4,
            kFeatureSendRecv2Zstd,
            kFeatureSendRecv2DryRunSend,
            kFeatureOpenscreenMdns,
    };

    return set;
}

std::string featureSetToString(const FeatureSet& set) {
    if (set.empty())
        return "";

    std::string str = set[0];
    for (size_t i = 1; i < set.size(); ++i)
        str += ',' + set[i];

    return str;
}

int main()
{
    auto context = LibusbContext::makeContext();


    auto optTransport = [&] () -> std::optional<UsbTransport> {
        auto list = context->getDeviceVector();
        if (list.empty()) {
            std::cerr << "Cannot get device list" << std::endl;
            return {};
        }

        for (auto& device : list) {
            auto optTransport = UsbTransport::createTransport(device);
            if (optTransport) {
                auto deviceDescriptor = device.getDescriptor();
                std::cout << "Device found. VenID: " << deviceDescriptor->idVendor << ", ProdID: " << deviceDescriptor->idProduct
                          << std::endl;
                return optTransport;
            }
        }

        return {};
    }();

    auto thread = context->spawnEventHandlingThread();
    thread.detach();

    if (!optTransport) {
        std::cerr << "Cannot create transport" << std::endl;
        return 1;
    }


    try {
        auto& transport = *optTransport;

        APacket packet{};
        packet.setNewMessage(AMessage{.command = A_CNXN, .arg0 = A_VERSION, .arg1 = MAX_PAYLOAD_V1}, false);

        // stupid copy
        auto str = "host::features=" + featureSetToString(getFeatureSet());
        auto payload = std::make_unique<SimplePayload>(str.size());
        auto* payloadData = payload->getData();
        for (size_t i = 0; i < str.size(); ++i)
            payloadData[i] = str[i];

        packet.setNewPayload(payload.get(), true, false);

        std::mutex mutex{};
        std::condition_variable cv{};
        int libusbError = 0;

        // Create Listeners
        transport.setSendListener([&] (const APacket& sentPacket, int errorCode) {
            std::unique_lock lock(mutex);
            libusbError = errorCode;
            lock.unlock();
            cv.notify_one();
        });
        transport.setReceiveListener([&] (const APacket& receivedPacket, int errorCode) {
            std::unique_lock lock(mutex);
            libusbError = errorCode;
            packet.message = receivedPacket.message;    // copy head
            packet.payload = new SimplePayload(*dynamic_cast<SimplePayload*>(receivedPacket.payload));  // copy payload
            lock.unlock();
            cv.notify_one();
        });

        std::unique_lock lock(mutex);
        std::cout << "Sending..." << std::endl;
        transport.write(packet);
        cv.wait(lock);

        if (libusbError != LibusbTransfer::COMPLETED) {
            std::cerr << "Send error code: " << libusbError << std::endl;
            return 1;
        }

        std::cout << "Receiving..." << std::endl;
        transport.receive();
        cv.wait(lock);
        if (libusbError != LibusbTransfer::COMPLETED) {
            std::cerr << "Send error code: " << libusbError << std::endl;
            return 1;
        }

        std::cout << "Head: " << packet.message.command << std::endl;
        std::cout << "APayload (size: " << packet.message.dataLength << "): "<< std::endl;
        if (packet.payload != nullptr) {
            std::cout << "\t";
            for(size_t i = 0; i < packet.payload->getLength(); ++i)
                std::cout << packet.payload->getData()[i];
            std::cout << std::endl;
        }

    }
    catch (LibusbError& error) {
        OBJLIBUSB_IOSTREAM_REPORT_ERROR(std::cerr, error);
    }


    return 0;
}
