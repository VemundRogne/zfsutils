#include "zfsutils/dataset.hpp"

#include <CLI/CLI.hpp>
#include <cerrno>
#include <chrono>
#include <format>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <errno.h>

#include <optional>

int main(int argc, char **argv) {
    auto logger = spdlog::stdout_color_mt("snapper");
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_default_logger(logger);

    CLI::App app{"ZFSUTILS -- snapper"};
    argv = app.ensure_utf8(argv);

    std::string dataset;
    app.add_option("--dataset", dataset, "dataset to snapshot")->required();
    CLI11_PARSE(app, argc, argv);

    auto target_dataset = zfsutils::Dataset::open(dataset);
    if (!target_dataset) {
        spdlog::error("Could not open dataset '{}'", dataset);
        return 0;
    }

    // This timestamp is UTC 'YYYY-MM-DDThh:mmZ' where appended "Z" means UTC
    std::string timestamp =
        std::format("{:%FT%R}Z", std::chrono::system_clock::now());

    try {
        auto snap = target_dataset->createSnapshot(timestamp);
        spdlog::info("Created snapshot: {}", snap.fullName());
    } catch (std::logic_error &e) {
        spdlog::error("Dataset {} already has a snapshot with timestamp {}",
                      target_dataset->name(), timestamp);
        return -EEXIST;
    } catch (std::runtime_error &e) {
        spdlog::error(
            "Could not snapshot dataset '{}'; perhaps a permissions-issue?",
            target_dataset->name());
        return -EACCES;
    }

    return 0;
}
