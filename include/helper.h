#pragma once
#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 35


#include "helloworld.grpc.pb.h"
#include <utime.h>
#include <sys/stat.h>



using helloworld::BasicRPC;
using helloworld::Int;
using helloworld::Stat;
using helloworld::PathNFlag;

namespace helloworld {
    inline bool operator==(const Time& a, const Time& b) {
        return a.sec() == b.sec() &&  a.nsec() == b.nsec();
    }
}

constexpr bool DISABLE_CERR_ERRORS = false;
constexpr bool PRINT_SERVER_OUT = true;

inline std::pair<int, std::string> get_tmp_file() {
    char templat[100];
    strcpy(templat, "/tmp/afs_tmp_fileXXXXXX");
    const int fd = mkstemp(templat);
    return {fd, std::string(templat)};
}

static uint64_t get_mod_ts(const char* path) {
    struct stat st;
    const int ret = ::stat(path, &st);
    if (ret < 0) st.st_mtim.tv_sec = 0;
    return st.st_mtim.tv_sec;
}

inline void print_ts(const helloworld::Time& ts) {
    std::cerr << "[" << ts.sec() << "." << ts.nsec() << "] ";
}
inline void print_ts(const uint64_t ts) {
    std::cerr << "[" << ts << "]\n";
}

inline void print_proto_stat(const Stat& st) {
    std::cerr << "ts: ";
    print_ts(st.atim());
    print_ts(st.mtim());
    print_ts(st.ctim());
    std::cerr << " " << st.blocks() << "\n";
}

template <class... T>
inline void cerr_errors(const T&... args) {
    if constexpr (!DISABLE_CERR_ERRORS)
        (std::cerr << ... << args) << '\n';
}

template <class... T>
inline void log_client(const T&... args) {
    if constexpr (!DISABLE_CERR_ERRORS)
        (std::cerr << ... << args) << '\n';
}


template <class ReplyT>
inline void print_server_out(const char* fn, const ReplyT& reply) {
    if constexpr (PRINT_SERVER_OUT)
        (std::cerr << fn << " -> " << reply.get_value() << "\n");
}

constexpr bool CERR_SERVER_CALLS = true;

template <class... T>
inline void cerr_serv_calls(const T&... args) {
    if constexpr (CERR_SERVER_CALLS)
        (std::cerr << ... << args) << '\n';
}

enum class FileStatus: int {
    OK = 1,
    FILE_OPEN_ERROR,
    FILE_ALREADY_CACHED,
};

inline std::string get_port_from_env() {
    const char* ENV_VAR = "GRPC_PORT";
    const char* ret = std::getenv(ENV_VAR);
    if (ret == NULL) {
        std::cerr <<  ENV_VAR << " env var is not set. aborting\n";
        std::abort();
    }
    return std::string(std::getenv(ENV_VAR));
}
