#pragma once

#include "../dataset/PartitionCsvParser.hpp"
#include "../dataset/PartitionStore.hpp"
#include "../model/LocalQueryResult.hpp"
#include "../model/QueryRequest.hpp"

/*
 * LocalQueryEngine
 * ----------------
 * Worker-side execution engine
 *
 * Responsibilities:
 * - scan this worker's assigned files on demand
 * - parse rows during query execution
 * - apply local filter predicates
 * - return local COUNT or EXECUTE results
 */
class LocalQueryEngine
{
public:
    LocalQueryResult count(const PartitionStore &store,
                           const QueryRequest &request) const;

    LocalQueryResult execute(const PartitionStore &store,
                             const QueryRequest &request) const;

private:
    bool matchesRow(const ParsedPartitionRow &row,
                    const QueryRequest &request) const;
};