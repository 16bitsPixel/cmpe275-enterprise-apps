import argparse
from concurrent import futures

import grpc
from google.protobuf import empty_pb2

import basecamp_pb2
import basecamp_pb2_grpc


def load_config(path: str) -> dict:
    cfg = {
        "node_id": "",
        "language": "",
        "listen_host": "",
        "listen_port": 0,
        "neighbors": []
    }

    with open(path, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            if "=" not in line:
                raise RuntimeError(f"Invalid config line: {line}")

            key, value = line.split("=", 1)
            key = key.strip()
            value = value.strip()

            if key == "node_id":
                cfg["node_id"] = value
            elif key == "language":
                cfg["language"] = value
            elif key == "listen_host":
                cfg["listen_host"] = value
            elif key == "listen_port":
                cfg["listen_port"] = int(value)
            elif key == "neighbors":
                cfg["neighbors"] = []
                if value:
                    for item in value.split(","):
                        item = item.strip()
                        if not item:
                            continue
                        node_and_host, port_str = item.rsplit(":", 1)
                        node_id, host = node_and_host.split("@", 1)
                        cfg["neighbors"].append({
                            "node_id": node_id.strip(),
                            "host": host.strip(),
                            "port": int(port_str.strip())
                        })
            else:
                raise RuntimeError(f"Unknown config key: {key}")

    if not cfg["node_id"]:
        raise RuntimeError("Config missing node_id")
    if cfg["language"] not in ("cpp", "python"):
        raise RuntimeError("Config language must be cpp or python")
    if not cfg["listen_host"]:
        raise RuntimeError("Config missing listen_host")
    if not cfg["listen_port"]:
        raise RuntimeError("Config missing listen_port")

    return cfg


class NodeService(basecamp_pb2_grpc.NodeServiceServicer):
    def __init__(self, cfg: dict):
        self.cfg = cfg

    def Ping(self, request, context):
        print(f"[rpc] Ping() on node {self.cfg['node_id']}", flush=True)
        return basecamp_pb2.PingReply(
            node_id=self.cfg["node_id"],
            host=self.cfg["listen_host"],
            port=self.cfg["listen_port"],
            language=self.cfg["language"],
        )

    def GatherNeighbors(self, request, context):
        print(
            f"[rpc] GatherNeighbors() on node {self.cfg['node_id']} count={len(self.cfg['neighbors'])}",
            flush=True,
        )
        reply = basecamp_pb2.NeighborReply(node_id=self.cfg["node_id"])
        for n in self.cfg["neighbors"]:
            reply.neighbors.append(
                basecamp_pb2.NeighborInfo(
                    node_id=n["node_id"],
                    target=f"{n['host']}:{n['port']}",
                )
            )
        return reply

    def ForwardProbe(self, request, context):
        print(
            f"[rpc] ForwardProbe() node={self.cfg['node_id']} "
            f"origin={request.origin_id} ttl={request.ttl} payload=\"{request.payload}\"",
            flush=True,
        )

        # Milestone 1 Python node keeps this simple: acknowledge receipt.
        message = (
            f"node={self.cfg['node_id']}, ttl={request.ttl}, "
            f"fanout={len(self.cfg['neighbors'])}"
        )

        return basecamp_pb2.ProbeReply(
            responder_id=self.cfg["node_id"],
            accepted=True,
            message=message,
        )


def serve(config_path: str) -> None:
    cfg = load_config(config_path)

    server = grpc.server(futures.ThreadPoolExecutor(max_workers=8))
    basecamp_pb2_grpc.add_NodeServiceServicer_to_server(NodeService(cfg), server)

    listen_target = f"{cfg['listen_host']}:{cfg['listen_port']}"
    server.add_insecure_port(listen_target)

    print("----------------------------------------", flush=True)
    print("Python node server ready", flush=True)
    print(f"node_id   : {cfg['node_id']}", flush=True)
    print(f"language  : {cfg['language']}", flush=True)
    print(f"listen    : {listen_target}", flush=True)
    print(f"neighbors : {len(cfg['neighbors'])}", flush=True)
    print("----------------------------------------", flush=True)

    server.start()
    server.wait_for_termination()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", required=True)
    args = parser.parse_args()
    serve(args.config)