#pragma once

#include <cstddef>
#include <string>

/*
 * RowRef
 * ------
 * Identifies one matching row in the distributed system.
 *
 * Used for global ordering:
 * - globalFileOrder: which file this row belongs to (global position)
 * - localRowId: row index within that file
 */
struct RowRef
{
    std::string nodeId;
    std::size_t localRowId = 0;

    RowRef() = default;

    RowRef(const std::string &nodeId_, std::size_t localRowId_)
        : nodeId(nodeId_), localRowId(localRowId_)
    {
    }
};