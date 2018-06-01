/** @file
 *  @brief Загаловочный файл модуля классов узла
 *  @author Petin Yuriy recycle@List.ru
 *  @date   created 15.07.2017
 *  @date   modified 15.07.2017
 *  @version 1.0 (alpha)
 */
#ifndef NODE_H
#define NODE_H
#include "common.h"
#include "syncro.h"
#include "pipeprocessor.h"

namespace MultiPipeProcessor
{
typedef TMultiPipeProcessor_t TValue;

using _TNode = TNode<TValue>;
typedef _TNode* TKey;
using TList = std::list<TValue>;

template <typename TContainer = TList>
struct TLinkedNode :  _TNode
{
    // In order to avoid empty node processing
    // linked list gear is used
    // Node with usefull data is included in list
    // Empty node is excluded from list
    std::atomic<bool> isLinked; // Exclisive lock required
    TLinkedNode * next;     // Exclusive lock required
    TLinkedNode * prev;     // Exclusive lock required
    TContainer * m_list; // Data node lock required
};


}

#endif // NODE_H
