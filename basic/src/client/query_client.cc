#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "../transport/QueryProtoConverters.hpp"
#include "query.grpc.pb.h"

static void printUsage(const char* prog) {
    std::cerr
        << "Usage:\n"
        << "  " << prog << " --target <host:port> --submit [filters]\n"
        << "  " << prog << " --target <host:port> --fetch --request-id <id> --max-rows <n>\n"
        << "  " << prog << " --target <host:port> --cancel --request-id <id>\n"
        << "\n"
        << "Submit filters:\n"
        << "  --payment-type <int>\n"
        << "  --distance-lo <float> --distance-hi <float>\n"
        << "  --total-lo <int> --total-hi <int>\n"
        << "  --pickup-lo <int64> --pickup-hi <int64>\n"
        << "  --chunk-size <n>\n";
}

static std::unique_ptr<mini2::query::QueryService::Stub> makeStub(const std::string& target) {
    auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    return mini2::query::QueryService::NewStub(channel);
}

int main(int argc, char** argv) {
    try {
        std::string target;
        bool doSubmit = false;
        bool doFetch = false;
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
            } else if (arg == "--fetch") {
                doFetch = true;
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

            mini2::query::SubmitQueryReply resp;
            grpc::ClientContext ctx;
            ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(3000));

            grpc::Status status = stub->SubmitQuery(&ctx, req, &resp);
            if (!status.ok()) {
                std::cerr << "SubmitQuery failed: " << status.error_message() << "\n";
                return 4;
            }

            std::cout << "SubmitQuery ok\n";
            std::cout << "accepted   : " << (resp.accepted() ? "true" : "false") << "\n";
            std::cout << "request_id : " << resp.request_id() << "\n";
            std::cout << "node_id    : " << resp.node_id() << "\n";
            std::cout << "message    : " << resp.message() << "\n";
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
            ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(3000));

            grpc::Status status = stub->FetchChunk(&ctx, req, &resp);
            if (!status.ok()) {
                std::cerr << "FetchChunk failed: " << status.error_message() << "\n";
                return 6;
            }

            std::cout << "FetchChunk ok\n";
            std::cout << "found         : " << (resp.found() ? "true" : "false") << "\n";
            std::cout << "request_id    : " << resp.request_id() << "\n";
            std::cout << "node_id       : " << resp.node_id() << "\n";
            std::cout << "rows_returned : " << resp.rows_returned() << "\n";
            std::cout << "done          : " << (resp.done() ? "true" : "false") << "\n";
            std::cout << "message       : " << resp.message() << "\n";

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
            ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(2000));

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