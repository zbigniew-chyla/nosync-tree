#ifndef NOSYNC__TREE__NODES_DATA_COLLECTING_REQUEST_HANDLER_H
#define NOSYNC__TREE__NODES_DATA_COLLECTING_REQUEST_HANDLER_H

#include <functional>
#include <memory>
#include <nosync/event-loop.h>
#include <nosync/request-handler.h>
#include <nosync/tree/node.h>
#include <string>


namespace nosync::tree
{

template<typename NodeExtData, typename NodeData>
std::shared_ptr<request_handler<std::string, node<NodeData>>> make_nodes_data_collecting_request_handler(
    event_loop &evloop,
    std::shared_ptr<request_handler<std::string, NodeExtData>> node_ext_data_provider,
    std::function<void(const NodeExtData &, const std::function<void(const std::string &)> &)> &&ext_data_for_each_child_func,
    std::function<NodeData(NodeExtData &&)> &&node_data_factory);

}

#include <nosync/tree/nodes-data-collecting-request-handler-impl.h>

#endif
