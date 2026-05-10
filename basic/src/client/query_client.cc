#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "../transport/QueryProtoConverters.hpp"
#include "query.grpc.pb.h"

static constexpr int RPC_DEADLINE_SECONDS = 600;

static void printUsage(const char* prog) {
    std::cerr
        << "Usage:\n"
        << "  " << prog << " --target <host:port> --submit [--count] [filters]\n"
        << "  " << prog << " --target <host:port> --fetch --request-id <id> --max-rows <n>\n"
        << "  " << prog << " --target <host:port> --fetch-all --request-id <id> [--max-rows <n>]\n"
        << "  " << prog << " --target <host:port> --cancel --request-id <id>\n"
        << "\n"
        << "Submit filters:\n"
        << "  --payment-type <int>\n"
        << "  --distance-lo <float> --distance-hi <float>\n"
        << "  --total-lo <int> --total-hi <int>\n"
        << "  --pickup-lo <int64> --pickup-hi <int64>\n"
        << "  --chunk-size <n>\n"
        << "  --count\n";
}

static std::unique_ptr<mini2::query::QueryService::Stub> makeStub(const std::string& target) {
    auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    return mini2::query::QueryService::NewStub(channel);
}

static void printRows(const mini2::query::FetchChunkReply& resp) {
    for (int i = 0; i < resp.rows_size(); ++i) {
        const auto& row = resp.rows(i);
        std::cout
            << "row[" << i << "] "
            << "row_id=" << row.row_id()
            << " source=" << row.source_node_id()
            << " pickup=" << row.pickup_datetime()
            << " distance=" << row.trip_distance()
            << " total=" << row.total_amount()
            << "\n";
    }
}

int main(int argc, char** argv) {
    try {
        std::string target;
        bool doSubmit = false;
        bool doFetch = false;
        bool doFetchAll = false;
        bool doCancel = false;

        QueryRequest submitReq("", QueryType::Execute);
        std::string requestId;
        uint32_t maxRows = 64;

        bool hasDistanceLo = false, hasDistanceHi = false;
        float distanceLo = 0.0f, distanceHi = 0.0f;

        bool hasTotalLo = false, hasTotalHi = false;
        int32_t totalLo = 0, totalHi = 0;

        bool hasPickupLo = false, hasPickupHi = false;
        int64_t pickupLo = 0, pickupHi = 0;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--target" && i + 1 < argc) {
                target = argv[++i];
            } else if (arg == "--submit") {
                doSubmit = true;
            } else if (arg == "--count") {
                submitReq.setQueryType(QueryType::Count);
            } else if (arg == "--fetch") {
                doFetch = true;
            } else if (arg == "--fetch-all") {
                doFetchAll = true;
            } else if (arg == "--cancel") {
                doCancel = true;
            } else if (arg == "--request-id" && i + 1 < argc) {
                requestId = argv[++i];
            } else if (arg == "--max-rows" && i + 1 < argc) {
                maxRows = static_cast<uint32_t>(std::stoul(argv[++i]));
            } else if (arg == "--chunk-size" && i + 1 < argc) {
                submitReq.chunkSize = static_cast<uint32_t>(std::stoul(argv[++i]));
            } else if (arg == "--payment-type" && i + 1 < argc) {
                submitReq.paymentType = static_cast<int32_t>(std::stoi(argv[++i]));
            } else if (arg == "--distance-lo" && i + 1 < argc) {
                distanceLo = std::stof(argv[++i]);
                hasDistanceLo = true;
            } else if (arg == "--distance-hi" && i + 1 < argc) {
                distanceHi = std::stof(argv[++i]);
                hasDistanceHi = true;
            } else if (arg == "--total-lo" && i + 1 < argc) {
                totalLo = static_cast<int32_t>(std::stoi(argv[++i]));
                hasTotalLo = true;
            } else if (arg == "--total-hi" && i + 1 < argc) {
                totalHi = static_cast<int32_t>(std::stoi(argv[++i]));
                hasTotalHi = true;
            } else if (arg == "--pickup-lo" && i + 1 < argc) {
                pickupLo = static_cast<int64_t>(std::stoll(argv[++i]));
                hasPickupLo = true;
            } else if (arg == "--pickup-hi" && i + 1 < argc) {
                pickupHi = static_cast<int64_t>(std::stoll(argv[++i]));
                hasPickupHi = true;
            } else {
                printUsage(argv[0]);
                return 1;
            }
        }

        if (target.empty()) {
            printUsage(argv[0]);
            return 1;
        }

        const int modeCount =
            static_cast<int>(doSubmit) +
            static_cast<int>(doFetch) +
            static_cast<int>(doFetchAll) +
            static_cast<int>(doCancel);

        if (modeCount != 1) {
            std::cerr << "Choose exactly one mode: --submit, --fetch, --fetch-all, or --cancel\n";
            return 1;
        }

        if (hasDistanceLo != hasDistanceHi) {
            std::cerr << "distance range requires both --distance-lo and --distance-hi\n";
            return 2;
        }
        if (hasTotalLo != hasTotalHi) {
            std::cerr << "total range requires both --total-lo and --total-hi\n";
            return 2;
        }
        if (hasPickupLo != hasPickupHi) {
            std::cerr << "pickup range requires both --pickup-lo and --pickup-hi\n";
            return 2;
        }

        if (hasDistanceLo && hasDistanceHi) {
            submitReq.tripDistanceRange = Range<float>{distanceLo, distanceHi};
        }
        if (hasTotalLo && hasTotalHi) {
            submitReq.totalAmountRange = Range<int32_t>{totalLo, totalHi};
        }
        if (hasPickupLo && hasPickupHi) {
            submitReq.pickupRange = Range<int64_t>{pickupLo, pickupHi};
        }
        if (submitReq.chunkSize == 0) {
            submitReq.chunkSize = 64;
        }

        auto stub = makeStub(target);
        if (!stub) {
            std::cerr << "Failed to create stub\n";
            return 3;
        }

        if (doSubmit) {
            mini2::query::SubmitQueryRequest req;

            if (submitReq.pickupRange) {
                req.mutable_filter()->mutable_pickup_range()->set_lo(submitReq.pickupRange->lo);
                req.mutable_filter()->mutable_pickup_range()->set_hi(submitReq.pickupRange->hi);
            }
            if (submitReq.dropoffRange) {
                req.mutable_filter()->mutable_dropoff_range()->set_lo(submitReq.dropoffRange->lo);
                req.mutable_filter()->mutable_dropoff_range()->set_hi(submitReq.dropoffRange->hi);
            }
            if (submitReq.tripDistanceRange) {
                req.mutable_filter()->mutable_distance_range()->set_lo(submitReq.tripDistanceRange->lo);
                req.mutable_filter()->mutable_distance_range()->set_hi(submitReq.tripDistanceRange->hi);
            }
            if (submitReq.totalAmountRange) {
                req.mutable_filter()->mutable_total_cents_range()->set_lo(submitReq.totalAmountRange->lo);
                req.mutable_filter()->mutable_total_cents_range()->set_hi(submitReq.totalAmountRange->hi);
            }
            if (submitReq.tipAmountRange) {
                req.mutable_filter()->mutable_tip_cents_range()->set_lo(submitReq.tipAmountRange->lo);
                req.mutable_filter()->mutable_tip_cents_range()->set_hi(submitReq.tipAmountRange->hi);
            }
            if (submitReq.paymentType) {
                req.mutable_filter()->set_payment_type(*submitReq.paymentType);
            }

            req.set_preferred_chunk_size(static_cast<uint32_t>(submitReq.chunkSize));
            req.set_query_type(
                submitReq.getQueryType() == QueryType::Count
                    ? mini2::query::QUERY_COUNT
                    : mini2::query::QUERY_EXECUTE
            );

            mini2::query::SubmitQueryReply resp;
            grpc::ClientContext ctx;
            ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(RPC_DEADLINE_SECONDS));

            auto start = std::chrono::high_resolution_clock::now();
            grpc::Status status = stub->SubmitQuery(&ctx, req, &resp);
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double, std::milli> submitMs = end - start;

            if (!status.ok()) {
                std::cerr << "SubmitQuery failed: " << status.error_message() << "\n";
                return 4;
            }

            std::cout << "SubmitQuery ok\n";
            std::cout << "accepted   : " << (resp.accepted() ? "true" : "false") << "\n";
            std::cout << "request_id : " << resp.request_id() << "\n";
            std::cout << "node_id    : " << resp.node_id() << "\n";
            std::cout << "message    : " << resp.message() << "\n";
            std::cout << "submit_ms  : " << submitMs.count() << "\n";
            return 0;
        }

        if (doFetch) {
            if (requestId.empty()) {
                std::cerr << "--fetch requires --request-id\n";
                return 5;
            }

            mini2::query::FetchChunkRequest req;
            req.set_request_id(requestId);
            req.set_max_rows(maxRows);

            mini2::query::FetchChunkReply resp;
            grpc::ClientContext ctx;
            ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(RPC_DEADLINE_SECONDS));

            auto start = std::chrono::high_resolution_clock::now();
            grpc::Status status = stub->FetchChunk(&ctx, req, &resp);
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double, std::milli> fetchMs = end - start;

            if (!status.ok()) {
                std::cerr << "FetchChunk failed: " << status.error_message() << "\n";
                return 6;
            }

            printRows(resp);

            std::cout << "FetchChunk ok\n";
            std::cout << "found         : " << (resp.found() ? "true" : "false") << "\n";
            std::cout << "request_id    : " << resp.request_id() << "\n";
            std::cout << "node_id       : " << resp.node_id() << "\n";
            std::cout << "rows_returned : " << resp.rows_returned() << "\n";
            std::cout << "rows_scanned  : " << resp.rows_scanned() << "\n";
            std::cout << "rows_matched  : " << resp.rows_matched() << "\n";
            std::cout << "done          : " << (resp.done() ? "true" : "false") << "\n";
            std::cout << "message       : " << resp.message() << "\n";
            std::cout << "fetch_ms      : " << fetchMs.count() << "\n";
            return 0;
        }

        if (doFetchAll) {
            if (requestId.empty()) {
                std::cerr << "--fetch-all requires --request-id\n";
                return 5;
            }

            const uint32_t fetchSize = maxRows == 0 ? 1000 : maxRows;

            bool done = false;
            std::uint64_t totalRows = 0;
            std::uint64_t chunks = 0;
            std::uint64_t lastRowsScanned = 0;
            std::uint64_t lastRowsMatched = 0;

            double firstFetchMs = 0.0;
            double totalFetchRpcMs = 0.0;

            auto fetchAllStart = std::chrono::high_resolution_clock::now();

            while (!done) {
                mini2::query::FetchChunkRequest req;
                req.set_request_id(requestId);
                req.set_max_rows(fetchSize);

                mini2::query::FetchChunkReply resp;
                grpc::ClientContext ctx;
                ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(RPC_DEADLINE_SECONDS));

                auto chunkStart = std::chrono::high_resolution_clock::now();
                grpc::Status status = stub->FetchChunk(&ctx, req, &resp);
                auto chunkEnd = std::chrono::high_resolution_clock::now();

                std::chrono::duration<double, std::milli> chunkMs = chunkEnd - chunkStart;

                if (!status.ok()) {
                    std::cerr << "FetchChunk failed: " << status.error_message() << "\n";
                    return 6;
                }

                ++chunks;
                if (chunks == 1) {
                    firstFetchMs = chunkMs.count();
                }

                totalFetchRpcMs += chunkMs.count();
                totalRows += resp.rows_returned();
                lastRowsScanned = resp.rows_scanned();
                lastRowsMatched = resp.rows_matched();
                done = resp.done();

                std::cout << "chunk=" << chunks
                          << " rows=" << resp.rows_returned()
                          << " done=" << (done ? "true" : "false")
                          << " chunk_ms=" << chunkMs.count()
                          << "\n";

                if (resp.rows_returned() == 0 && !done) {
                    std::cerr << "Fetch returned 0 rows but done=false; stopping to avoid infinite loop\n";
                    return 9;
                }
            }

            auto fetchAllEnd = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> fetchAllMs = fetchAllEnd - fetchAllStart;

            const double avgFetchMs =
                chunks == 0 ? 0.0 : totalFetchRpcMs / static_cast<double>(chunks);

            std::cout << "\nEXECUTE BENCHMARK\n";
            std::cout << "request_id       : " << requestId << "\n";
            std::cout << "chunk_size       : " << fetchSize << "\n";
            std::cout << "chunks           : " << chunks << "\n";
            std::cout << "rows_returned    : " << totalRows << "\n";
            std::cout << "rows_scanned     : " << lastRowsScanned << "\n";
            std::cout << "rows_matched     : " << lastRowsMatched << "\n";
            std::cout << "first_fetch_ms   : " << firstFetchMs << "\n";
            std::cout << "avg_fetch_ms     : " << avgFetchMs << "\n";
            std::cout << "fetch_all_ms     : " << fetchAllMs.count() << "\n";
            return 0;
        }

        if (doCancel) {
            if (requestId.empty()) {
                std::cerr << "--cancel requires --request-id\n";
                return 7;
            }

            mini2::query::CancelQueryRequest req;
            req.set_request_id(requestId);

            mini2::query::CancelQueryReply resp;
            grpc::ClientContext ctx;
            ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(RPC_DEADLINE_SECONDS));

            grpc::Status status = stub->CancelQuery(&ctx, req, &resp);
            if (!status.ok()) {
                std::cerr << "CancelQuery failed: " << status.error_message() << "\n";
                return 8;
            }

            std::cout << "CancelQuery ok\n";
            std::cout << "success    : " << (resp.success() ? "true" : "false") << "\n";
            std::cout << "request_id : " << resp.request_id() << "\n";
            std::cout << "node_id    : " << resp.node_id() << "\n";
            std::cout << "message    : " << resp.message() << "\n";
            return 0;
        }

        printUsage(argv[0]);
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 10;
    }
}


