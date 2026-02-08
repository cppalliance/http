#include <boost/http.hpp>

int main() {
  boost::http::request request;
  request.set_payload_size(1337);
  if (request.payload_size() != 1337) {
    throw;
  }
}
