#ifndef NOSYNC__TREE__UTILS_IMPL_H
#define NOSYNC__TREE__UTILS_IMPL_H

#include <algorithm>
#include <nosync/tree/node.h>

namespace nosync::tree
{

template<typename DataType, typename DataCompareFunc>
void sort_recursively(node<DataType> &tree, const DataCompareFunc &cmp_func)
{
    std::stable_sort(
        tree.children.begin(), tree.children.end(),
        [&](const node<DataType> &node_a, const node<DataType> &node_b) {
            return cmp_func(node_a.data, node_b.data);
        });

    for (node<DataType> &child : tree.children) {
        sort_recursively(child, cmp_func);
    }
}

}

#endif
