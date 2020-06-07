
#include "server.hpp"
#include <asio.hpp>
#include <asio/steady_timer.hpp>
#include <iostream>
#include <memory>
#include <functional>

class Server final: public
sdbus::AdaptorInterfaces<
  net::Corp::MyApp::Frobber_adaptor> {
public:
  Server(sdbus::IConnection &connection, std::string objectpath) :
    AdaptorInterfaces<net::Corp::MyApp::Frobber_adaptor>(connection, std::move(objectpath)) {
    this->registerAdaptor();
  }

  virtual ~Server() {
    this->unregisterAdaptor();
  }
  bool Verbose() override {
    return verbose;
  }

  void Verbose(const bool& value) override {
    verbose = value;
  }

  std::string HelloWorld(const std::string &greeting) override {
    return "Hello " + greeting;
  }

  bool verbose = false;
};


class SdBusIOHandler :
  public std::enable_shared_from_this<SdBusIOHandler>
{
public:
  SdBusIOHandler(std::shared_ptr<asio::io_service> io_service, std::shared_ptr<sdbus::IConnection> connection) :
    io_service(io_service)
    , connection(connection)
    , descriptor(*io_service, connection->getEventLoopPollData().fd)
  {
    auto ec = std::error_code();
  }

  void handle(std::error_code ec, std::size_t) {
    if (!ec) {
      std::cout << "Process Pending requests\n";
      connection->processPendingRequest();
      descriptor.async_read_some(asio::null_buffers(),
                                 [me = shared_from_this()](auto ec, auto s) {
                                   me->handle(ec, s);
                                 });
    }
  }

  std::shared_ptr<asio::io_service> io_service;
  std::shared_ptr<sdbus::IConnection> connection;
  asio::posix::stream_descriptor descriptor;
};

class PeriodicTimer :
  public std::enable_shared_from_this<PeriodicTimer>
{
public:
  PeriodicTimer(asio::io_service *io_service, const std::function<void() > f)
    : f(f), timer(*io_service)
  {};

  void activate() {
    timer.expires_from_now(duration);
    timer.async_wait([me = shared_from_this()](auto &ec) {me->handle(ec);});
  }

  void handle(std::error_code ec) {
    if (!ec) {
      f();
      activate();
    }
  }

  const std::function<void()> f;
  asio::steady_timer timer;
  std::chrono::duration<int64_t> duration = std::chrono::seconds(5);
};

int main() {
  const auto objectpath = "/net/corp/myapp";
  const auto servicName = "net.corp.myapp";


  std::shared_ptr<sdbus::IConnection> connection = sdbus::createSessionBusConnection(servicName);
  // std::shared_ptr<sdbus::IObject> object = sdbus::createObject(*connection, objectpath);

  auto server = std::make_shared<Server>(*connection, objectpath);


  std::shared_ptr<asio::io_service> io_service  = std::make_shared<asio::io_service>();
  std::shared_ptr<SdBusIOHandler> handler = std::make_shared<SdBusIOHandler>(io_service, connection);
  handler->handle(std::error_code(), 0);


  auto timer = std::make_shared<PeriodicTimer>(io_service.get(),
                                               [server]() {
                                                 std::cout << "Timer expired\n";
                                                 server->emitNotification({1}, 3, {"Foo"});});
  timer->activate();


  // asio::steady_timer timer(*io_service);
  // timer.expires_from_now(std::chrono::seconds(5));
  // timer.async_wait(

  asio::io_service::work work(*io_service);
  io_service->run();


  return 0;
}
