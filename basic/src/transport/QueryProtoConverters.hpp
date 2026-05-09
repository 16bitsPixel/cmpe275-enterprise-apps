#pragma once

#include <string>

#include "../model/QueryRequest.hpp"
#include "../model/TaxiTrip.hpp"
#include "query.pb.h"

namespace QueryProtoConverters {

// request conversion
QueryRequest fromProtoSubmitQueryRequest(const mini2::query::SubmitQueryRequest& in);
QueryRequest fromProtoSubmitSubQueryRequest(const mini2::query::SubmitSubQueryRequest& in);

// reply helpers
void fillSubmitQueryReply(bool accepted,
                          const std::string& requestId,
                          const std::string& nodeId,
                          const std::string& message,
                          mini2::query::SubmitQueryReply* out);

void fillSubmitSubQueryReply(bool accepted,
                             const std::string& requestId,
                             const std::string& nodeId,
                             const std::string& message,
                             mini2::query::SubmitSubQueryReply* out);

void fillCancelQueryReply(bool success,
                          const std::string& requestId,
                          const std::string& nodeId,
                          const std::string& message,
                          mini2::query::CancelQueryReply* out);

void fillCancelSubQueryReply(bool success,
                             const std::string& requestId,
                             const std::string& nodeId,
                             const std::string& message,
                             mini2::query::CancelSubQueryReply* out);

// row conversion
mini2::query::TripRow toProtoTripRow(const TaxiTrip& trip, const std::string& sourceNodeId);

} // namespace QueryProtoConverters