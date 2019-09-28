#include <filesystem>
#include <functional>
#include <iostream>
#include <iterator>
#include <nosync/event-loop-utils.h>
#include <nosync/os/synchronized-queue-based-event-loop.h>
#include <nosync/os/thread-pool-executor.h>
#include <nosync/os/threaded-request-handler.h>
#include <nosync/raw-error-result.h>
#include <nosync/tree/ascii-print.h>
#include <nosync/tree/nodes-data-collecting-request-handler.h>
#include <nosync/tree/utils.h>
#include <string>
#include <utility>
#include <vector>


namespace
{

struct dir_info
{
    std::string name;
    std::vector<std::string> sub_dirs_names;
};


std::shared_ptr<nosync::request_handler<std::string, dir_info>> make_dir_info_provider(
    std::function<void(std::function<void()>)> evloop_mt_executor,
    std::function<void(std::function<void()>)> bg_thread_executor)
{
    auto dir_info_provider = nosync::os::make_threaded_request_handler<std::string, dir_info>(
        std::move(evloop_mt_executor), std::move(bg_thread_executor),
        [](std::string path, nosync::eclock::duration) -> nosync::result<dir_info> {
            namespace fs = std::filesystem;

            std::error_code dir_ec;
            fs::directory_iterator dir_iter(path, dir_ec);
            if (dir_ec) {
                return nosync::raw_error_result(dir_ec);
            }

            std::vector<std::string> sub_dirs_names;
            for (const auto dir_entry : dir_iter) {
                if (dir_entry.is_directory()) {
                    sub_dirs_names.push_back(dir_entry.path().filename());
                }
            }

            return nosync::make_ok_result(
                dir_info{
                    fs::path(path).filename(),
                    std::move(sub_dirs_names)});
        });

    return dir_info_provider;
}


void start_app(
    nosync::os::mt_event_loop &evloop,
    std::function<void(std::function<void()>)> bg_thread_executor)
{
    auto dir_info_tree_provider = nosync::tree::make_nodes_data_collecting_request_handler<dir_info, std::string>(
        evloop,
        make_dir_info_provider(evloop.get_mt_executor(), bg_thread_executor),
        [](const auto &info, const auto &child_func) {
            for (const auto &child_name : info.sub_dirs_names) {
                child_func(child_name);
            }
        },
        [](dir_info &&info) {
            return std::move(info.name);
        });

    dir_info_tree_provider->handle_request(
        ".", nosync::eclock::duration::max(),
        [](auto res) {
            if (res.is_ok()) {
                auto &dir_tree = res.get_value();
                nosync::tree::sort_recursively(dir_tree);
                nosync::tree::print_ascii_tree(
                    dir_tree,
                    [](const auto prefix, const auto &name) {
                        std::cout << prefix << name << "\n";
                    });
            } else {
                std::cerr << "error: " << res.get_error().message() << "\n";
            }
        });
}

}


int main()
{
    auto mt_evloop = nosync::os::make_synchronized_queue_based_event_loop();
    auto &evloop = *mt_evloop;

    {
        auto bg_thread_executor = nosync::os::make_thread_pool_executor(1);
        invoke_later(
            evloop,
            [&evloop, bg_thread_executor]() {
                start_app(evloop, bg_thread_executor);
            });
    }

    mt_evloop->run_iterations();

    return 0;
}
