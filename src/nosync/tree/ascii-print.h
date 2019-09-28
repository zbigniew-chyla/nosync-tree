#ifndef NOSYNC__TREE__ASCII_PRINT_H
#define NOSYNC__TREE__ASCII_PRINT_H

#include <cstddef>
#include <nosync/tree/node.h>


namespace nosync::tree
{

template<typename DataType, typename PrintFunc>
void print_ascii_tree(
    const node<DataType> &tree, std::size_t indent_line_size, std::size_t pad_size,
    const PrintFunc &line_print_func);

template<typename DataType, typename PrintFunc>
void print_ascii_tree(const node<DataType> &tree, const PrintFunc &line_print_func);

}

#include <nosync/tree/ascii-print-impl.h>

#endif
