#include "manifest_parser.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace ara {
namespace exec {
namespace parser {

using json = nlohmann::json;

bool ExecutionManifest::operator==(const ExecutionManifest& other) const noexcept
{
    return (manifest_id == other.manifest_id) && (processes == other.processes);
}

bool ExecutionManifest::operator!=(const ExecutionManifest& other) const noexcept
{
    return !(*this == other);
}

bool ExecutionManifest::Process::operator==(const Process& other) const noexcept
{
    return (name == other.name) && (startup_configs == other.startup_configs);
}

bool ExecutionManifest::Process::operator!=(const Process& other) const noexcept
{
    return !(*this == other);
}

bool ExecutionManifest::Process::StartupConfig::operator==(
    const StartupConfig& other) const noexcept
{
    return (startup_options == other.startup_options) &&
           (machine_instance_refs == other.machine_instance_refs);
}

bool ExecutionManifest::Process::StartupConfig::operator!=(
    const StartupConfig& other) const noexcept
{
    return !(*this == other);
}

bool ExecutionManifest::Process::StartupConfig::StartupOption::operator==(
    const StartupOption& other) const noexcept
{
    return (kind == other.kind) && (name == other.name) && (arg == other.arg);
}

bool ExecutionManifest::Process::StartupConfig::StartupOption::operator!=(
    const StartupOption& other) const noexcept
{
    return !(*this == other);
}

bool ExecutionManifest::Process::StartupConfig::MachineInstanceRef::operator==(
    const MachineInstanceRef& other) const noexcept
{
    return (function_group == other.function_group) && (mode == other.mode);
}

bool ExecutionManifest::Process::StartupConfig::MachineInstanceRef::operator!=(
    const MachineInstanceRef& other) const noexcept
{
    return !(*this == other);
}

namespace {
namespace EMJsonKeys {
const std::string kApplicationManifest = "Application_manifest";
const std::string kApplicationManifestId = "Application_manifest_id";
const std::string kProcess = "Process";
const std::string kProcessName = "Process_name";
const std::string kModeDependentStartupConfigs = "Mode_dependent_startup_configs";
const std::string kStartupOptions = "Startup_options";
const std::string kStartupOptionsOptionKind = "Option_kind";
const std::string kStartupOptionsOptionName = "Option_name";
const std::string kStartupOptionsOptionArg = "Option_arg";
const std::string kModeInMachineInstanceRefs = "Mode_in_machine_instance_refs";
const std::string kFunctionGroup = "Function_group";
const std::string kMode = "Mode";

const std::vector<std::string> kAsVector{kApplicationManifest,
                                         kApplicationManifestId,
                                         kProcess,
                                         kProcessName,
                                         kModeDependentStartupConfigs,
                                         kStartupOptions,
                                         kStartupOptionsOptionKind,
                                         kStartupOptionsOptionName,
                                         kStartupOptionsOptionArg,
                                         kModeInMachineInstanceRefs,
                                         kFunctionGroup,
                                         kMode};

}  // namespace EMJsonKeys

namespace MMJsonKeys {
const std::string kMachineManifest = "Machine_manifest";
// TODO add rest of the keys

const std::vector<std::string> kAsVector{kMachineManifest /* TODO add rest of the keys */};
}  // namespace MMJsonKeys

}  // anonymous namespace

template <typename T>
bool try_to_read_value(const json& json_obj, const std::string& key, T& output_value) noexcept
{
    T tmp{};
    try {
        tmp = json_obj.at(key);
    } catch (json::exception& e) {
        std::cerr << "ara::exec::parser::try_to_read_value -> " << e.what()
                  << '\n';  // development purposes
        return false;
    }
    output_value = tmp;
    return true;
}

ExecutionManifest ManifestParser::parse_execution_manifest(const std::string& path) noexcept(false)
{
    using namespace EMJsonKeys;
    auto manifest_json_full = read_manifest_file(path);
    ExecutionManifest man{};
    validate_content(manifest_json_full, kAsVector);

    json manifest_json_content{};
    try_to_read_value(manifest_json_full, kApplicationManifest, manifest_json_content);

    try_to_read_value(manifest_json_content, kApplicationManifestId, man.manifest_id);

    json processes{};
    if (try_to_read_value(manifest_json_content, kProcess, processes)) {
        for (auto& process : processes) {
            ExecutionManifest::Process proc{};
            try_to_read_value(process, kProcessName, proc.name);

            json startup_configs{};
            if (try_to_read_value(process, kModeDependentStartupConfigs, startup_configs)) {
                for (auto& startup_config : startup_configs) {
                    ExecutionManifest::Process::StartupConfig config{};

                    json startup_options{};
                    if (try_to_read_value(startup_config, kStartupOptions, startup_options)) {
                        for (auto& startup_option : startup_options) {
                            ExecutionManifest::Process::StartupConfig::StartupOption option{};

                            try_to_read_value(startup_option, kStartupOptionsOptionKind,
                                              option.kind);
                            try_to_read_value(startup_option, kStartupOptionsOptionName,
                                              option.name);
                            try_to_read_value(startup_option, kStartupOptionsOptionArg, option.arg);

                            config.startup_options.push_back(option);
                        }
                    }

                    json machine_instance_refs{};
                    if (try_to_read_value(startup_config, kModeInMachineInstanceRefs,
                                          machine_instance_refs)) {
                        for (auto& machine_instance_ref : machine_instance_refs) {
                            ExecutionManifest::Process::StartupConfig::MachineInstanceRef
                                mach_inst_ref{};
                            try_to_read_value(machine_instance_ref, kFunctionGroup,
                                              mach_inst_ref.function_group);
                            try_to_read_value(machine_instance_ref, kMode, mach_inst_ref.mode);

                            config.machine_instance_refs.push_back(mach_inst_ref);
                        }
                    }
                    proc.startup_configs.push_back(config);
                }
            }
            man.processes.push_back(proc);
        }
    }
    return man;
}

MachineManifest ManifestParser::parse_machine_manifest(const std::string& path) noexcept(false)
{
    using namespace MMJsonKeys;
    auto manifest_json{read_manifest_file(path)};

    validate_content(manifest_json, kAsVector);

    // TODO implement rest of the method

    return MachineManifest();
}

json ManifestParser::read_manifest_file(const std::string& path) noexcept(false)
{
    std::ifstream manifest_content(path, std::ifstream::binary);
    if (not manifest_content.is_open()) {
        throw std::runtime_error(
            "ara::com::exec::parser::parse_manifest -> cannot open manifest: " + path + "\n");
    }

    json manifest_json{};

    try {
        manifest_content >> manifest_json;
    } catch (json::exception& e) {
        manifest_content.close();
        throw std::runtime_error(
            "ManifestParser::read_manifest_file -> cannot read manifest file. " +
            std::string(e.what()));
    }

    manifest_content.close();
    return manifest_json;
}

void ManifestParser::validate_content(const json& /*json_obj*/,
                                      const std::vector<std::string>& /*json_keys*/) const
    noexcept(false)
{
    // If validation will end with failure, this method will throw std::runtime_error
}

}  // namespace parser
}  // namespace exec
}  // namespace ara
