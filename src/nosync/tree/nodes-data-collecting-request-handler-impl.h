#ifndef NOSYNC__TREE__NODES_DATA_COLLECTING_REQUEST_HANDLER_IMPL_H
#define NOSYNC__TREE__NODES_DATA_COLLECTING_REQUEST_HANDLER_IMPL_H

#include <algorithm>
#include <nosync/deadline-setting-request-handler.h>
#include <nosync/destroy-notifier.h>
#include <nosync/function-utils.h>
#include <nosync/memory-utils.h>
#include <nosync/raw-error-result.h>
#include <nosync/result-handler-utils.h>
#include <nosync/time-utils.h>
#include <nosync/type-utils.h>
#include <optional>
#include <system_error>
#include <utility>


namespace nosync::tree
{

namespace nodes_data_collecting_request_handler_impl
{

template<typename NodeData>
node<NodeData> make_node_from_collected_child_nodes(
    NodeData &&data, std::vector<std::pair<std::size_t, node<NodeData>>> &&collected_child_nodes)
{
    using std::move;

    std::sort(
        collected_child_nodes.begin(), collected_child_nodes.end(),
        [](const auto &lhs, const auto &rhs) {
            return lhs.first < rhs.first;
        });

    std::vector<node<NodeData>> child_nodes;
    child_nodes.reserve(collected_child_nodes.size());
    for (auto &child_node_entry : collected_child_nodes) {
        child_nodes.push_back(move(child_node_entry.second));
    }

    return {move(data), move(child_nodes)};
}


template<typename NodeExtData, typename NodeData>
class nodes_data_collecting_request_handler :
    public nosync::request_handler<std::string, node<NodeData>>,
    public std::enable_shared_from_this<nodes_data_collecting_request_handler<NodeExtData, NodeData>>
{
public:
    nodes_data_collecting_request_handler(
        nosync::event_loop &evloop,
        std::shared_ptr<nosync::request_handler<std::string, NodeExtData>> node_ext_data_provider,
        std::function<void(const NodeExtData &, const std::function<void(const std::string &)> &)> &&ext_data_for_each_child_func,
        std::function<NodeData(NodeExtData &&)> &&node_data_factory);

    void handle_request(
        std::string &&request, nosync::eclock::duration timeout,
        nosync::result_handler<node<NodeData>> &&response_handler) override;

private:
    struct request_context
    {
        std::error_code first_error;
        std::shared_ptr<nosync::request_handler<std::string, NodeExtData>> node_ext_data_provider;
    };

    void collect_subtree_data(
        const std::string &path, const std::shared_ptr<request_context> &ctx,
        nosync::result_handler<node<NodeData>> res_handler) const;

    nosync::event_loop &evloop;
    std::shared_ptr<nosync::request_handler<std::string, NodeExtData>> node_ext_data_provider;
    std::function<void(const NodeExtData &, const std::function<void(const std::string &)> &)> ext_data_for_each_child_func;
    std::function<NodeData(NodeExtData &&)> node_data_factory;
};


template<typename NodeExtData, typename NodeData>
nodes_data_collecting_request_handler<NodeExtData, NodeData>::nodes_data_collecting_request_handler(
    nosync::event_loop &evloop,
    std::shared_ptr<nosync::request_handler<std::string, NodeExtData>> node_ext_data_provider,
    std::function<void(const NodeExtData &, const std::function<void(const std::string &)> &)> &&ext_data_for_each_child_func,
    std::function<NodeData(NodeExtData &&)> &&node_data_factory)
    : evloop(evloop), node_ext_data_provider(std::move(node_ext_data_provider)),
    ext_data_for_each_child_func(std::move(ext_data_for_each_child_func)),
    node_data_factory(std::move(node_data_factory))
{
}


template<typename NodeExtData, typename NodeData>
void nodes_data_collecting_request_handler< NodeExtData, NodeData>::handle_request(
    std::string &&request, nosync::eclock::duration timeout, nosync::result_handler<node<NodeData>> &&res_handler)
{
    auto ctx = move_to_shared(
        request_context{
            std::error_code(),
            make_deadline_setting_request_handler(
                evloop, make_copy(node_ext_data_provider),
                time_point_sat_add(evloop.get_etime(), timeout))});

    collect_subtree_data(move(request), ctx, move(res_handler));
}


template<typename NodeExtData, typename NodeData>
void nodes_data_collecting_request_handler<NodeExtData, NodeData>::collect_subtree_data(
    const std::string &path, const std::shared_ptr<request_context> &ctx,
    nosync::result_handler<node<NodeData>> res_handler) const
{
    using namespace nosync;
    using std::move;

    if (ctx->first_error) {
        invoke_result_handler_later(evloop, move(res_handler), raw_error_result(ctx->first_error));
        return;
    }

    ctx->node_ext_data_provider->handle_request(
        make_copy(path), eclock::duration::max(),
        [self = this->shared_from_this(), res_handler = move(res_handler), ctx, path](auto ext_data_res) {
            if (!ext_data_res.is_ok()) {
                if (!ctx->first_error) {
                    ctx->first_error = ext_data_res.get_error();
                }
                res_handler(raw_error_result(ext_data_res));
                return;
            }

            auto node_data_ptr = std::make_shared<std::optional<NodeData>>();
            auto collected_child_nodes = std::make_shared<std::vector<std::pair<std::size_t, node<NodeData>>>>();
            auto return_node_destroy_notifier = make_shared_destroy_notifier(
                [res_handler = move(res_handler), ctx, node_data_ptr, collected_child_nodes]() {
                    res_handler(
                        !ctx->first_error
                            ? make_ok_result(
                                make_node_from_collected_child_nodes(
                                    move(**node_data_ptr), move(*collected_child_nodes)))
                            : raw_error_result(ctx->first_error));
                });

            auto &node_ext_data = ext_data_res.get_value();
            self->ext_data_for_each_child_func(
                node_ext_data,
                make_enumerating_func(
                    [&](std::size_t child_index, const std::string &child_name) {
                        self->collect_subtree_data(
                            path + "/" + child_name, ctx,
                            [child_index, collected_child_nodes, return_node_destroy_notifier](auto node_res) {
                                if (node_res.is_ok()) {
                                    collected_child_nodes->emplace_back(child_index, move(node_res.get_value()));
                                }
                            });
                    }));

            *node_data_ptr = self->node_data_factory(move(node_ext_data));
        });
}

}


template<typename NodeExtData, typename NodeData>
std::shared_ptr<nosync::request_handler<std::string, node<NodeData>>> make_nodes_data_collecting_request_handler(
    nosync::event_loop &evloop,
    std::shared_ptr<nosync::request_handler<std::string, NodeExtData>> node_ext_data_provider,
    std::function<void(const NodeExtData &, const std::function<void(const std::string &)> &)> &&ext_data_for_each_child_func,
    std::function<NodeData(NodeExtData &&)> &&node_data_factory)
{
    using namespace nodes_data_collecting_request_handler_impl;
    using std::move;

    return std::make_shared<nodes_data_collecting_request_handler<NodeExtData, NodeData>>(
        evloop, move(node_ext_data_provider), move(ext_data_for_each_child_func), move(node_data_factory));
}

}

#endif
