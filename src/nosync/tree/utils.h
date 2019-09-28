#ifndef NOSYNC__TREE__UTILS_H
#define NOSYNC__TREE__UTILS_H

#include <functional>
#include <nosync/tree/node.h>


namespace nosync::tree
{

template<typename DataType, typename DataCompareFunc = std::less<DataType>>
void sort_recursively(node<DataType> &tree, const DataCompareFunc &cmp_func = std::less<DataType>());

}

#include <nosync/tree/utils-impl.h>

#endif
