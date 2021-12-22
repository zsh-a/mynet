#include "mynet/connection.h"
#include "mynet/task.h"
namespace mynet {
Task<Connection::Buffer> Connection::read(ssize_t size) {
  if (size < 0) co_return co_await read_until_eof();
  Buffer res(size);
  co_await channel_.read(loop_);
  ssize_t tot_read = ::read(fd_, res.data(), size);
  if (tot_read < 0) {
    perror("read error");
    // exit(tot_read);
    res.resize(0);
    co_return res;
  }
  res.resize(tot_read);
  co_return res;
}

Task<Connection::Buffer> Connection::readn(ssize_t size) {
  if (size < 0) co_return co_await read_until_eof();
  ssize_t tot_read = 0;
  Buffer res(size);
  while (tot_read < size) {
    co_await channel_.read(loop_);
    ssize_t ret = ::read(fd_, res.data() + tot_read, size - tot_read);
    if (ret == 0) break;
    if (ret < 0) {
      perror("read error");
      // exit(tot_read);
      res.resize(0);
      co_return res;
    }
    tot_read += ret;
  }
  res.resize(tot_read);
  // fmt::print("tot_read read {} bytes\n",tot_read);
  co_return res;
}

Task<bool> Connection::write(const Buffer& buf) {
  ssize_t tot_writen = 0;
  while (tot_writen < buf.size()) {
    co_await channel_.write(loop_);
    ssize_t writen =
        ::write(fd_, buf.data() + tot_writen, buf.size() - tot_writen);
    if (writen < 0) {
      perror("write data failed");
      co_return false;
      // exit(errno);
    }
    tot_writen += writen;
  }
  co_return true;
}

Task<Connection::Buffer> Connection::read_until_eof() {
  int tot_read = 0;
  Buffer res;
  for (int cur_read = 0;;) {
    co_await channel_.read(loop_);
    Buffer buf(BUFFER_SIZE);
    cur_read = ::read(fd_, buf.data(), BUFFER_SIZE);
    if (cur_read < 0) {
      perror("read error");
      exit(cur_read);
    }
    // fmt::print("read {} bytes return {} \n",tot_read,cur_read);
    if (cur_read == 0) break;
    buf.resize(cur_read);
    res.insert(res.end(), buf.begin(), buf.end());
    tot_read += cur_read;
  }

  co_return res;
}

}  // namespace mynet