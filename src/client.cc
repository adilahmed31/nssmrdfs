#include "client.h"
#include <future>

#include <fuse.h>
#include <sys/stat.h>
#include <unistd.h>

void sigintHandler(int sig_num)
{
    std::cerr << "Clean Shutdown\n";
    //    if (srv_ptr) {
    //        delete srv_ptr;
    //    }
    fflush(stdout);
    std::exit(0);
}

std::unique_ptr<BasicRPCClient> greeter;
int do_getattr(const char* path, struct stat* st) {
    const Stat s = greeter->c_stat(path);
    st->st_ino = s.ino();
    st->st_mode = s.mode();
    st->st_nlink = s.nlink();
    st->st_uid = s.uid();
    st->st_gid = s.gid();
    st->st_rdev = s.rdev();
    st->st_size = s.size();
    st->st_blocks = s.blocks();
    auto get_time = [&](const helloworld::Time& t) {
        struct timespec ret;
        ret.tv_sec = t.sec();
        ret.tv_nsec = t.nsec();
        return ret;
    };
    st->st_atim = get_time(s.atim());
    st->st_mtim = get_time(s.mtim());
//    st->st_ctim = get_time(s.ctim());
//	if ( strcmp( path, "/" ) == 0 )
//	{
//		st->st_mode = S_IFDIR | 0755;
//		st->st_nlink = 2;
//	}
//	else
//	{
//		st->st_mode = S_IFREG | 0644;
//		st->st_nlink = 1;
//		st->st_size = 1024;
//	}
    //std::cerr << "[*] set mode for " << path << " is " << st->st_mode << "\n";
		
    return s.error();
}

int do_fgetattr(const char* path, struct stat* stbuf,  fuse_file_info*) {
    std::cerr << __PRETTY_FUNCTION__ << '\n';
    return do_getattr(path, stbuf);
}
static void *hello_init(struct fuse_conn_info *conn)
{
    std::cerr << __PRETTY_FUNCTION__ << std::endl;
	return NULL;
}

static int do_open(const char* path, struct fuse_file_info* fi) {
    std::cerr << __PRETTY_FUNCTION__ << '\n';
    fi->fh = greeter->c_open(path, fi->flags);
    return 0;
}
int do_mkdir(const char* path, mode_t mode) {
    return greeter->c_mkdir(path);
}
int do_rmdir(const char* path) {
    return greeter->c_rmdir(path);
}

static int do_create(const char* path, mode_t mode, struct fuse_file_info* fi){
     std::cerr << __PRETTY_FUNCTION__ << '\n';
     if (int ret = greeter->c_create(path, fi->flags); ret < 0)
        return ret;
    const int fd = fi->fh = greeter->c_open(path, fi->flags);
    if (fd < 0) return fd;
    return 0;
}

static int do_access(const char* path, int) {
    std::cerr << __PRETTY_FUNCTION__ << '\n';
    return 0;
}

static int do_read(const char* path, char* buf,
    size_t size, off_t offset, struct  fuse_file_info *fi){
    int rc = 0;
    rc = pread(fi->fh,buf,size,offset);
    
    if(rc<0){
        return -errno;
    }

    return rc;
}

static int do_write(const char* path, const char* buf,
        size_t size, off_t offset, struct  fuse_file_info *fi){

    const int rc = pwrite(fi->fh,buf,size,offset);
    std::cerr << __PRETTY_FUNCTION__ << path << " " << rc << "\n";
    if(rc<0){
        return -errno;
    }

    return rc;
}

static int do_opendir(const char* path, struct fuse_file_info* fi){
    std::cerr << __PRETTY_FUNCTION__ << '\n';
    return 0;
}

static int do_releasedir(const char* path, struct fuse_file_info* fi){
    std::cerr << __PRETTY_FUNCTION__ << '\n';
    return 0;
}

int do_unlink(const char* path) {
    return greeter->c_unlink(path);
}
int do_readdir(const char* path, void* buffer, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info* fi) {
    const auto resp = greeter->c_readdir(path);
    if (auto err = resp.ret_code(); err < 0)
        return err;

    for (auto &s : resp.names()) {
        filler(buffer, s.c_str(), nullptr, 0);
    }
    return 0;
}

static int do_release(const char* path, struct fuse_file_info* fi) {
    std::cerr <<"closing file now..\n";
    greeter->c_release(path, fi->fh);
    return ::close(fi->fh);
}

void test() {
    usleep(1e6);
    const char* fname = "/tmp/ab_fs/b.txt";
    int fd = ::open(fname, O_CREAT| O_RDWR);
    std::cerr << "open w fd:"  << fd << "\n";
    ::write(fd, fname, strlen(fname));
    ::close(fd);
    // O_APPEND
    std::cerr << "reopening file for append mode now\n";
    fd = ::open(fname, O_RDWR);
//    ::write(fd, fname, strlen(fname));
//    ::write(fd, fname, strlen(fname));
    ::pwrite(fd, "ABH", 3, 0);
    ::close(fd);
//    greeter->c_create("a.txt", 0777);
//    print_proto_stat(greeter->c_stat("a.txt"));

    //std::cerr << "trying to open fuse file /tmp/ab_fuse/a.txt\n";
//    std::cerr << "[*] opening file\n";
//    FILE* fi = fopen("/tmp/ab_fs/a.txt", "r");
//    int fd = fileno(fi);
//    std::cerr << "fd is " << fd << "\n";
//    char filePath[PATH_MAX];
//    if (fcntl(fd, F_GETPATH, filePath) != -1)
//        std::cerr << "path for the file is : " << filePath << "\n";

//    char buf[100];
//    std::cerr << "[*] calling read now...\n";
//    const int sz = read(fd, buf, sizeof(buf));
//    std::cerr << buf;
}
static struct fuse_operations operations;
int main(int argc, char *argv[])
{
    // "ctrl-C handler"
    signal(SIGINT, sigintHandler);
    const std::string target_str = "localhost:" + get_port_from_env();
    grpc::ChannelArguments ch_args;

    ch_args.SetMaxReceiveMessageSize(INT_MAX);
    ch_args.SetMaxSendMessageSize(INT_MAX);

    greeter = std::make_unique<BasicRPCClient>(
            grpc::CreateCustomChannel(target_str,
            grpc::InsecureChannelCredentials() , ch_args ));

    auto tester = std::async(std::launch::async, [&]() { test(); });
    operations.create = do_create;
    operations.init = hello_init;
    operations.open = do_open;
    operations.getattr = do_getattr;
    operations.readdir = do_readdir;
//    operations.access = do_access;
    operations.read = do_read;
    operations.write = do_write;
    operations.opendir = do_opendir;
    operations.readdir = do_readdir;
    operations.releasedir = do_releasedir;
//    operations.fgetattr = do_fgetattr;
    operations.release = do_release;
    operations.mkdir = do_mkdir;
    operations.rmdir = do_rmdir;
    operations.unlink = do_unlink;
    return fuse_main(argc, argv, &operations, &greeter);
}
