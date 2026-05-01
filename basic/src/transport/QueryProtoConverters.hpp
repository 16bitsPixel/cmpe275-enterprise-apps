#pragma once

#include <string>
#include <vector>

#include "../model/QueryRequest.hpp"
#include "../model/QueryResult.hpp"
#include "query.pb.h"

namespace QueryProtoConverters {

// ---------- QueryRequest / QueryFilter ----------

mini2::query::QueryFilter toProtoFilter(const QueryRequest& in);
QueryRequest fromProtoFilter(const mini2::query::QueryFilter& in);

// ---------- SubmitQuery ----------

mini2::query::SubmitQueryRequest toProtoSubmitQueryRequest(const QueryRequest& in);
QueryRequest fromProtoSubmitQueryRequest(const mini2::query::SubmitQueryRequest& in);

// ---------- SubmitSubQuery ----------

mini2::query::SubmitSubQueryRequest toProtoSubmitSubQueryRequest(
    const QueryRequest& in,
    const std::string& parentRequestId,
    const std::string& originNodeId
);

QueryRequest fromProtoSubmitSubQueryRequest(const mini2::query::SubmitSubQueryRequest& in);

// ---------- TripRow / QueryResultRow ----------

mini2::query::TripRow toProtoTripRow(const QueryResultRow& in);
QueryResultRow fromProtoTripRow(const mini2::query::TripRow& in);

void appendProtoRows(const std::vector<QueryResultRow>& rows,
                     google::protobuf::RepeatedPtrField<mini2::query::TripRow>* out);

std::vector<QueryResultRow> fromProtoRows(
    const google::protobuf::RepeatedPtrField<mini2::query::TripRow>& rows
);

} // namespace QueryProtoConverters