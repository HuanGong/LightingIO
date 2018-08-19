///
/// main.cpp
/// 
/// Example for basic using.
/// 

#include <resp/all.hpp>
#include <vector>
#include <cassert>

int main()
{
  /// Create encoder.
  resp::encoder<resp::buffer> enc;
  
  /// Create decoder.
  resp::decoder dec;
  
  /// We use asio to communicate with redis server(suppose it runing at local).
  /**
   * asio::io_service ios;
   * asio::ip::tcp::socket skt(ios);
   * asio::ip::tcp::resolver reso(ios);
   * asio::connect(skt, reso.resolve({"127.0.0.1", "6379"}));
   */
   
  /// Encode redis cmd SET key value.
  /**
  * @note Return at least one buffer, may multi, if multi, 
  *   user could use scatter-gather io to send them, this is for optimizing copy.
  */
  std::vector<resp::buffer> buffers = enc.encode("SET", "key", "value");
  
  /// Make send buffers of asio. 
  /**
   * std::vector<asio::const_buffer> send_buffers(buffers.size());
   * for (size_t i=0; i<buffers.size(); ++i)
   * {
   *   send_buffers[i] = asio::buffer(buffers[i].data(), buffers[i].size());
   * }
   */
  
  /// Send buffers to redis server.
  /**
   * asio::write(skt, send_buffers);
   */
   
  /// Recv reply from redis server. 
  /**
   * char reply[64];
   * size_t s = skt.read_some(asio::buffer(reply, 64));
   */
  
  /// This is what reply from redis server we supposed for.
  char reply[64] = "+OK\r\n";
  size_t s = 5;
  
  /// Decode buffer from redis server's reply.
  /// It should be "OK".
  resp::result res = dec.decode(reply, s);
  assert(res == resp::completed);
  resp::unique_value rep = res.value();
  assert(rep.string() == "OK");
  
  return 0;
}
