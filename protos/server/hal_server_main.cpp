// hal_server_main.cpp — standalone HAL gRPC server entry point.
//
// Usage:
//   hal_server [<address>]
//   hal_server 0.0.0.0:50051      # default
//   hal_server 127.0.0.1:9090
//
// TLS configuration (environment variables):
//   HAL_TLS_CERT   — path to PEM-encoded server certificate
//   HAL_TLS_KEY    — path to PEM-encoded server private key
//   HAL_TLS_CA     — path to PEM-encoded CA certificate (enables mutual TLS)
//
// When HAL_TLS_CERT and HAL_TLS_KEY are set, the server uses TLS.
// When HAL_TLS_CA is also set, client certificates are required (mTLS).
// Without these variables the server falls back to insecure mode (development only).

#include "server/hal_service_impl.h"
#include "server/module_registry_all.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>

#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

static std::unique_ptr<grpc::Server> g_server;

static void OnSignal(int /*sig*/) {
    if (g_server) {
        std::cout << "\n[hal_server] shutting down...\n";
        g_server->Shutdown();
    }
}

// Read a file into a string. Returns empty on error.
static std::string readFile(const char* path) {
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::shared_ptr<grpc::ServerCredentials> buildCredentials() {
    const char* cert_path = std::getenv("HAL_TLS_CERT");
    const char* key_path  = std::getenv("HAL_TLS_KEY");
    const char* ca_path   = std::getenv("HAL_TLS_CA");

    if (cert_path && key_path) {
        std::string cert = readFile(cert_path);
        std::string key  = readFile(key_path);
        if (cert.empty() || key.empty()) {
            std::cerr << "[hal_server] ERROR: TLS cert/key files not readable. Aborting.\n";
            std::exit(1);
        }
        grpc::SslServerCredentialsOptions ssl_opts(
            ca_path ? GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY
                    : GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE);
        ssl_opts.pem_key_cert_pairs.push_back({key, cert});
        if (ca_path) {
            std::string ca = readFile(ca_path);
            if (ca.empty()) {
                std::cerr << "[hal_server] ERROR: TLS CA file not readable. Aborting.\n";
                std::exit(1);
            }
            ssl_opts.pem_root_certs = ca;
        }
        std::cout << "[hal_server] TLS enabled"
                  << (ca_path ? " (mTLS)" : "") << "\n";
        return grpc::SslServerCredentials(ssl_opts);
    }

    std::cerr << "[hal_server] WARNING: TLS not configured — running insecure mode. "
              << "Set HAL_TLS_CERT and HAL_TLS_KEY for production deployments.\n";
    return grpc::InsecureServerCredentials();
}

int main(int argc, char* argv[]) {
    const std::string address = (argc > 1) ? argv[1] : "0.0.0.0:50051";

    // Build the service with all 35 HAL modules registered.
    auto registry = hal::server::CreateDefaultRegistry();
    hal::server::HalServiceImpl service(std::move(registry));

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, buildCredentials());
    builder.RegisterService(&service);

    // Allow large messages — test data blobs can be sizable.
    builder.SetMaxReceiveMessageSize(64 * 1024 * 1024);  // 64 MB
    builder.SetMaxSendMessageSize(64 * 1024 * 1024);

    g_server = builder.BuildAndStart();
    if (!g_server) {
        std::cerr << "[hal_server] failed to start on " << address << "\n";
        return 1;
    }

    std::cout << "[hal_server] listening on " << address << "\n";
    std::signal(SIGINT,  OnSignal);
    std::signal(SIGTERM, OnSignal);

    g_server->Wait();
    return 0;
}
