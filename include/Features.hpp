#ifndef ADB_LIB_FEATURES_HPP
#define ADB_LIB_FEATURES_HPP

#include <vector>
#include <string>


using FeatureSet = std::vector<std::string>;

class Feature {
public:
    Feature() = delete;

    static const std::string shell2;
    static const std::string cmd;
    static const std::string stat2;
    static const std::string ls2;
    static const std::string libusb;
    static const std::string pushSync;
    static const std::string apex;
    static const std::string fixedPushMkdir;
    static const std::string abb;
    static const std::string fixedPushSymlinkTimestamp;
    static const std::string abbExec;
    static const std::string remountShell;
    static const std::string trackApp;
    static const std::string sendRecv2;
    static const std::string sendRecv2Brotli;
    static const std::string sendRecv2LZ4;
    static const std::string sendRecv2Zstd;
    static const std::string sendRecv2DryRunSend;
    static const std::string openscreenMdns;
};


class Features {
public:
    Features() = delete;

    static const FeatureSet& getFullSet();
    static std::string setToString(const FeatureSet& set);
    static FeatureSet stringToSet(const std::string_view& view);

};

#endif //ADB_LIB_FEATURES_HPP
