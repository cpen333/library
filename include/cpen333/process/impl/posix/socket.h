/**
 * @file
 * @brief POSIX implementation of a socket
 *
 */
#ifndef CPEN333_PROCESS_POSIX_SOCKET_H
#define CPEN333_PROCESS_POSIX_SOCKET_H

#include <string>
#include <cstdint>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "../../../util.h"

#ifndef CPEN333_SOCKET_DEFAULT_PORT
/**
 * @brief Default port for making connections
 */
#define CPEN333_SOCKET_DEFAULT_PORT 5120
#endif

/**
 * @brief Invalid socket when trying to form a connection
 */
#define INVALID_SOCKET  -1

/**
 * @brief Socket error when connecting/binding/listening
 */
#define SOCKET_ERROR -1

namespace cpen333 {
namespace process {

namespace posix {

// forward declaration so client can friend it
class socket_server;

/**
 * @brief Socket client
 *
 * POSIX/BSD implementation of a socket client.  The client is
 * NOT connected automatically.  To start the connection,
 * call the open() function.
 */
class socket {
 private:
  std::string server_;
  int port_;
  int socket_;

  bool open_;
  bool connected_;

  friend class socket_server;

  /**
   * @copydoc cpen333::process::windows::socket::__initialize()
   */
  void __initialize(const std::string& server, int port,
                    int socket, bool open, bool connected) {
    server_ = server;
    port_ = port;
    socket_ = socket;
    open_ = open;
    connected_ = connected;
  }

  /**
   * @copydoc cpen333::process::windows::socket::disconnect()
   */
  bool disconnect() {

    if (!connected_) {
      return false;
    }

    // shutdown the connection since no more data will be sent
    int status = ::shutdown(socket_, SHUT_WR);
    if (status != 0) {
      cpen333::perror("shutdown(...) failed");
      return false;
    }
    connected_ = false;
    return true;
  }

 public:
  /**
   * @copydoc cpen333::process::windows::socket::socket()
   */
  socket() : server_("localhost"), port_(CPEN333_SOCKET_DEFAULT_PORT),
                    socket_(INVALID_SOCKET), open_(false), connected_(false) {}

  /**
   * @copydoc cpen333::process::windows::socket::socket(const std::string&,int)
   */
  socket(const std::string& server, int port) :
      server_(server), port_(port),
      socket_(INVALID_SOCKET), open_(false), connected_(false) {}

 private:
  socket(const socket &) DELETE_METHOD;
  socket &operator=(const socket &) DELETE_METHOD;

 public:

  /**
   * @brief Move-constructor
   * @param other socket to move to *this
   */
  socket(socket&& other) {
    *this = std::move(other);
  }

  /**
   * @brief Move-assignment
   * @param other socket to move
   * @return reference to myself
   */
  socket &operator=(socket&& other) {
    __initialize(other.server_, other.port_, other.socket_, other.open_, other.connected_);
    other.server_ = "";
    other.port_ = 0;
    other.socket_ = INVALID_SOCKET;
    other.open_ = false;
    other.connected_ = false;
    return *this;
  }

  /**
   * @copydoc cpen333::process::windows::socket::~socket()
   */
  ~socket() {
    close();
  }

  /**
   * @copydoc cpen333::process::windows::socket::open()
   */
  bool open() {

    // don't open if already opened
    if (open_) {
      return false;
    }

    // don't open if illegal port
    if (port_ <= 0) {
      return false;
    }

    /* Obtain address(es) matching host/port */
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    struct addrinfo *addrresult;
    std::string strport = std::to_string(port_);
    int status = getaddrinfo(server_.c_str(), strport.c_str(),
                             &hints, &addrresult);
    if (status != 0) {
      cpen333::error(std::string("getaddrinfo(...) failed with error: ")
                         + std::to_string(status));
      return false;
    }

    // Attempt to connect to an address until one succeeds
    for (struct addrinfo* ptr = addrresult; ptr != NULL; ptr = ptr->ai_next) {

      // Create a SOCKET for connecting to server
      socket_ = ::socket(ptr->ai_family, ptr->ai_socktype,
                             ptr->ai_protocol);

      if (socket_ == INVALID_SOCKET) {
        continue;
      }

      // Connect to server.
      status = ::connect(socket_, ptr->ai_addr, ptr->ai_addrlen);
      if (status != SOCKET_ERROR) {
        break;
      }

      // failed to connect, close socket
      ::close(socket_);
      socket_ = INVALID_SOCKET;
    }
    freeaddrinfo(addrresult);

    if (socket_ == INVALID_SOCKET) {
      cpen333::error(std::string("Unable to connect to server: ")
                         + server_ + std::string(":") + strport);
      return false;
    }

    open_ = true;
    connected_ = true;
    return true;
  }

  /**
   * @copydoc cpen333::process::windows::socket::write(const std::string&)
   */
  bool write(const std::string& str) {
    return write(str.c_str(), str.length()+1);
  }

  /**
   * @copydoc cpen333::process::windows::socket::write(const void*, size_t)
   */
  bool write(const void* buff, size_t size) {

    if (!connected_) {
      return false;
    }

    // write all contents
    size_t nwrite = 0;
    const char* cbuff = (const char*)buff;

    while (nwrite < size) {
      auto lwrite = ::write(socket_, &cbuff[nwrite], size-nwrite);
      if (lwrite == -1) {
        cpen333::perror(std::string("write(...) to socket failed"));
        return false;
      }
      nwrite += lwrite;
    }

    return true;
  }

  /**
   * @copydoc cpen333::process::windows::socket::read(void*,size_t)
   */
  size_t read(void* buff, size_t size) {

    if (!open_) {
      return 0;
    }

    auto nread = ::read(socket_, buff, size);
    if (nread == -1) {
      cpen333::perror("write(...) to socket failed");
      return 0;
    }

    return (size_t)nread;
  }

  /**
   * @brief Reads all data up to the specified size from the pipe
   *
   * Read bytes from the head of the pipe, blocking if necessary until all bytes are read.
   *
   * @param buff memory address to fill with pipe contents
   * @param size number of bytes to read
   * @return true if read is successful, false if read is interrupted
   */
  bool read_all(void* buff, size_t size) {
    char* cbuff = (char*)buff;
    size_t nread = read(cbuff, size);
    while (nread < size) {
      auto lread = read(&cbuff[nread], size-nread);
      if (lread <= 0) {
        return false;
      }
      nread += lread;
    }
    return true;
  }

  /**
   * @copydoc cpen333::process::windows::socket::close()
   */
  bool close() {

    if (!open_) {
      return false;
    }
    if (connected_) {
      disconnect();
    }

    // cleanup
    ::close(socket_);
    socket_ = INVALID_SOCKET;
    open_ = false;
    return true;
  }

};

/**
 * @brief Socket server
 *
 * POSIX/BSD implementation of a socket server that listens
 * for connections.  The server is NOT started automatically.
 * To start listening for connections, call the open() function.
 */
class socket_server {
  int port_;
  int socket_;
  bool open_;

 public:

  /**
   * @copydoc cpen333::process::windows::socket_server::socket_server(int)
   */
  socket_server(int port = CPEN333_SOCKET_DEFAULT_PORT) :
      port_(port), socket_(INVALID_SOCKET), open_(false) {}

 private:
  socket_server(const socket_server &) DELETE_METHOD;
  socket_server(socket_server &&) DELETE_METHOD;
  socket_server &operator=(const socket_server &) DELETE_METHOD;
  socket_server &operator=(socket_server &&) DELETE_METHOD;

 public:
  /**
   * @copydoc cpen333::process::windows::socket_server::~socket_server()
   */
  ~socket_server() {
    close();
  }

  /**
   * @copydoc cpen333::process::windows::socket_server::open()
   */
  bool open() {

    if (open_){
      return false;
    }

    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    struct addrinfo *addrresult = NULL;
    std::string strport = std::to_string(port_);
    int status = getaddrinfo(NULL, strport.c_str(), &hints, &addrresult);
    if ( status != 0 ) {
      cpen333::error(std::string("getaddrinfo(...) failed with error: ")
                         + std::to_string(status));
      return false;
    }

    // Create a SOCKET for connecting to server
    socket_ = ::socket(addrresult->ai_family, addrresult->ai_socktype,
                     addrresult->ai_protocol);
    if (socket_ == INVALID_SOCKET) {
      cpen333::perror("socket(...) failed");
      freeaddrinfo(addrresult);
      return false;
    }

    // Setup the TCP listening socket
    status = bind( socket_, addrresult->ai_addr, (int)addrresult->ai_addrlen);
    if (status == SOCKET_ERROR) {
      cpen333::perror("bind(...) failed");
      freeaddrinfo(addrresult);
      ::close(socket_);
      socket_ = INVALID_SOCKET;
      return false;
    }

    if (port_ == 0) {
      socklen_t addrlen = addrresult->ai_addrlen;
      status = getsockname(socket_, addrresult->ai_addr, &addrlen);
      if(status == 0 ) {
        struct sockaddr_in* sin = (struct sockaddr_in*)(addrresult->ai_addr);
        port_ = ntohs(sin->sin_port);
      } else {
        cpen333::perror(std::string("getsockname(...) failed with error: ")
                            + std::to_string(status));
        ::close(socket_);
        socket_ = INVALID_SOCKET;
        return false;
      }
    }

    freeaddrinfo(addrresult);

    status = listen(socket_, SOMAXCONN);
    if (status == SOCKET_ERROR) {
      cpen333::perror("listen(...) failed");
      ::close(socket_);
      socket_ = INVALID_SOCKET;
      return false;
    }

    open_ = true;
    return true;
  }

  /**
   * @copydoc cpen333::process::windows::socket_server::accept()
   */
  bool accept(socket& client) {
    if (!open_) {
      return false;
    }

    int client_socket = INVALID_SOCKET;

    // Accept a client socket
    client_socket = ::accept(socket_, NULL, NULL);
    if (client_socket == INVALID_SOCKET) {
      cpen333::perror("accept(...) failed");
      return false;
    }

    client.close();
    client.__initialize("", -1, client_socket, true, true);

    return true;
  }

  /**
   * @copydoc cpen333::process::windows::socket_server::close()
   */
  bool close() {
    if (!open_) {
      return false;
    }

    ::close(socket_);
    socket_ = INVALID_SOCKET;
    open_ = false;
    return true;
  }

  /**
   * @copydoc cpen333::process::windows::socket_server::port()
   */
  int port() {
    return port_;
  }

  /**
   * @copydoc cpen333::process::windows::socket_server::address_lookup()
   */
  static std::vector<std::string> address_lookup() {

    std::vector<std::string> out;

    // host name
    char ac[80];
    if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR) {
      return out;
    }
    std::string hostname(ac);

    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *addrresult;
    int status = getaddrinfo(hostname.c_str(), NULL,
                             &hints, &addrresult);

    if (status != 0) {
      return out;
    }

    // loop through addresses
    char ipbuf[INET_ADDRSTRLEN];
    // Attempt to connect to an address until one succeeds
    for (struct addrinfo* ptr = addrresult; ptr != NULL; ptr = ptr->ai_next) {
      out.push_back(inet_ntop(AF_INET, &((struct sockaddr_in *)ptr->ai_addr)->sin_addr, ipbuf, sizeof(ipbuf)));
    }
    freeaddrinfo(addrresult);

    return out;
  }

};

} // posix

/**
 * @brief POSIX implementation of a socket client
 */
typedef posix::socket socket;

/**
 * @brief POSIX implementation of a socket server
 */
typedef posix::socket_server socket_server;


} // process
} // cpen333

// undef local macros
#undef INVALID_SOCKET
#undef SOCKET_ERROR

#endif
