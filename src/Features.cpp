#include <utils.hpp>
#include "Features.hpp"

const std::string Feature::shell2 = "shell_v2";
const std::string Feature::cmd = "cmd";
const std::string Feature::stat2 = "stat_v2";
const std::string Feature::ls2 = "ls_v2";
const std::string Feature::libusb = "libusb";
const std::string Feature::pushSync = "push_sync";
const std::string Feature::apex = "apex";
const std::string Feature::fixedPushMkdir = "fixed_push_mkdir";
const std::string Feature::abb = "abb";
const std::string Feature::fixedPushSymlinkTimestamp = "fixed_push_symlink_timestamp";
const std::string Feature::abbExec = "abb_exec";
const std::string Feature::remountShell = "remount_shell";
const std::string Feature::trackApp = "track_app";
const std::string Feature::sendRecv2 = "sendrecv_v2";
const std::string Feature::sendRecv2Brotli = "sendrecv_v2_brotli";
const std::string Feature::sendRecv2LZ4 = "sendrecv_v2_lz4";
const std::string Feature::sendRecv2Zstd = "sendrecv_v2_zstd";
const std::string Feature::sendRecv2DryRunSend = "sendrecv_v2_dry_run_send";
const std::string Feature::openscreenMdns = "openscreen_mdns";

const FeatureSet& Features::getFullSet()
{
    const static FeatureSet set = {
        Feature::shell2,
        Feature::cmd,
        Feature::stat2,
        Feature::ls2,
        Feature::fixedPushMkdir,
        Feature::apex,
        Feature::abb,
        Feature::fixedPushSymlinkTimestamp,
        Feature::abbExec,
        Feature::remountShell,
        Feature::trackApp,
        Feature::sendRecv2,
        Feature::sendRecv2Brotli,
        Feature::sendRecv2LZ4,
        Feature::sendRecv2Zstd,
        Feature::sendRecv2DryRunSend,
        Feature::openscreenMdns,
        };

    return set;
}

std::string Features::setToString(const FeatureSet& set)
{
    if (set.empty())
        return {};

    std::string str = set[0];
    for (size_t i = 1; i < set.size(); ++i)
        str += ',' + set[i];

    return str;
}

FeatureSet Features::stringToSet(const std::string_view& view)
{
    FeatureSet set;
    auto tokens = utils::tokenize(view, ",");

    for (const auto& token : tokens)
        set.emplace_back(token);

    return set;
}
