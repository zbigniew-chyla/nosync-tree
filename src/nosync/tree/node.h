#ifndef NOSYNC__TREE__NODE_H
#define NOSYNC__TREE__NODE_H

#include <vector>


namespace nosync::tree
{

template<typename DataType>
struct node
{
    DataType data;
    std::vector<node> children;
};

}

#endif
