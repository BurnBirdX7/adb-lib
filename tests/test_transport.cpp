#include <iostream>
#include <vector>
#include <mutex>
#include <memory>
#include <condition_variable>


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
    auto context = ObjLibusbContext::make();

    auto uniqueTransport = [&] () -> std::unique_ptr<UsbTransport> {
        auto list = context->getDeviceVector();
        if (list.empty()) {
            std::cerr << "Cannot get device list" << std::endl;
            return {};
        }

        for (auto& device : list) {
            auto uniqueTransport = UsbTransport::make(device);
            if (uniqueTransport) {
                auto deviceDescriptor = device.getDescriptor();
                std::cout << "Device found. VenID: " << deviceDescriptor->idVendor << ", ProdID: " << deviceDescriptor->idProduct
                          << std::endl;
                return uniqueTransport;
            }
        }

        return {};
    }();

    auto thread = context->spawnEventHandlingThread();
    thread.detach();

    if (!uniqueTransport) {
        std::cerr << "Cannot create transport" << std::endl;
        return 1;
    }


    try {
        auto& transport = *uniqueTransport;

        APacket packet{};
        packet.setMessage(AMessage::make(A_CNXN, A_VERSION_MIN, MAX_PAYLOAD_V1));

        // stupid copy
        auto str = "host::features=" + featureSetToString(getFeatureSet());
        auto len = str.size();
        auto payload = APayload(len);
        payload.setDataSize(len);
        for (size_t i = 0; i < len; ++i)
            payload[i] = str[i];

        packet.movePayloadIn(std::move(payload));
        packet.updateMessageDataLength();
        packet.computeChecksum();

        std::mutex mutex{};
        std::condition_variable cv{};
        int libusbError = 0;

        // Create Listeners
        transport.setSendListener([&] (const APacket* sentPacket, int errorCode) {
            std::unique_lock lock(mutex);
            libusbError = errorCode;
            lock.unlock();
            cv.notify_one();
        });
        transport.setReceiveListener([&] (const APacket* receivedPacket, int errorCode) {
            std::unique_lock lock(mutex);
            libusbError = errorCode;
            packet.setMessage(receivedPacket->getMessage());     // copy message
            packet.copyPayloadIn(receivedPacket->getPayload());  // copy payload
            lock.unlock();
            cv.notify_one();
        });

        std::unique_lock lock(mutex);
        std::cout << "Sending..." << std::endl;
        transport.send(APacket(packet)); // send copy
        cv.wait(lock);

        if (libusbError != ObjLibusbTransfer::COMPLETED) {
            std::cerr << "Send error code: " << libusbError << std::endl;
            return 1;
        }

        std::cout << "Receiving..." << std::endl;
        transport.receive();
        cv.wait(lock);
        if (libusbError != ObjLibusbTransfer::COMPLETED) {
            std::cerr << "Send error code: " << libusbError << std::endl;
            return 1;
        }

        std::cout << "Head: ";
        for(size_t i = 0; i < 4; ++i)
            std::cout << reinterpret_cast<unsigned char*>(&packet.getMessage().command)[i];
        std::cout << std::endl;

        std::cout << "APayload (size: " << packet.getMessage().dataLength << "): "<< std::endl;
        if (packet.hasPayload()) {
            auto size = packet.getPayload().getSize();
            for(size_t i = 0; i < size; ++i)
                std::cout << packet.getPayload()[i];
            std::cout << std::endl;
        }
    }
    catch (ObjLibusbError& error) {
        std::cerr << error.what() << std::endl;
    }

    return 0;
}
