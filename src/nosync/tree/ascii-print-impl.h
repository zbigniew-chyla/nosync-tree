#ifndef NOSYNC__TREE__ASCII_PRINT_IMPL_H
#define NOSYNC__TREE__ASCII_PRINT_IMPL_H

#include <algorithm>
#include <functional>
#include <iterator>
#include <string>
#include <type_traits>


namespace nosync::tree
{

namespace ascii_print_impl
{

template<typename PrintFunc>
class tree_printer
{
public:
    tree_printer(const PrintFunc &line_print_func, std::size_t indent_line_size, std::size_t pad_size);

    template<typename DataType>
    void print_tree(const node<DataType> &tree) const;

private:
    template<typename DataType>
    void print_tree_impl(
        const node<DataType> &tree, const std::string &base_prefix,
        const std::string &my_prefix, const std::string &child_prefix) const;

    const PrintFunc &line_print_func;
    std::string vline_branch_prefix;
    std::string vline_prefix;
    std::string vline_end_prefix;
    std::string filler_prefix;
};


template<typename PrintFunc>
tree_printer<PrintFunc>::tree_printer(
    const PrintFunc &line_print_func, std::size_t indent_line_size, std::size_t pad_size)
    : line_print_func(line_print_func), vline_branch_prefix(), vline_prefix(), vline_end_prefix(), filler_prefix()
{
    std::size_t prefix_size = 1 + indent_line_size + pad_size;

    vline_branch_prefix.assign(prefix_size, ' ');
    vline_prefix.assign(prefix_size, ' ');
    vline_end_prefix.assign(prefix_size, ' ');
    filler_prefix.assign(prefix_size, ' ');

    vline_branch_prefix.front() = '|';
    vline_prefix.front() = '|';
    vline_end_prefix.front() = '`';

    std::fill(vline_branch_prefix.begin() + 1, vline_branch_prefix.begin() + 1 + indent_line_size, '-');
    std::fill(vline_end_prefix.begin() + 1, vline_end_prefix.begin() + 1 + indent_line_size, '-');
}


template<typename PrintFunc>
template<typename DataType>
void tree_printer<PrintFunc>::print_tree(const node<DataType> &tree) const
{
    std::string empty_prefix;
    print_tree_impl(tree, empty_prefix, empty_prefix, empty_prefix);
}


template<typename PrintFunc>
template<typename DataType>
void tree_printer<PrintFunc>::print_tree_impl(
    const node<DataType> &tree, const std::string &base_prefix,
    const std::string &my_prefix, const std::string &child_prefix) const
{
    line_print_func(base_prefix + my_prefix, tree.data);

    if (!tree.children.empty()) {
        const auto child_base_prefix = base_prefix + child_prefix;

        for (auto child_it = tree.children.begin(); child_it != std::prev(tree.children.end()); ++child_it) {
            print_tree_impl(*child_it, child_base_prefix, vline_branch_prefix, vline_prefix);
        }

        print_tree_impl(tree.children.back(), child_base_prefix, vline_end_prefix, filler_prefix);
    }
}

}


template<typename DataType, typename PrintFunc>
void print_ascii_tree(
    const node<DataType> &tree, std::size_t indent_line_size, std::size_t pad_size,
    const PrintFunc &line_print_func)
{
    using namespace ascii_print_impl;

    static_assert(
        std::is_assignable_v<std::function<void(const std::string &, const DataType &)>, PrintFunc>,
        "PrintFunc must be a function object accepting string prefix and node data");

    tree_printer(line_print_func, indent_line_size, pad_size).print_tree(tree);
}


template<typename DataType, typename PrintFunc>
void print_ascii_tree(const node<DataType> &tree, const PrintFunc &line_print_func)
{
    print_ascii_tree(tree, 2, 1, line_print_func);
}

}

#endif
