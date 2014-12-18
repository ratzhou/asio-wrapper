
#pragma once


#include <string>

#include <boost\asio.hpp>
#include <boost\bind.hpp>
#include <boost\shared_ptr.hpp>
#include <boost\enable_shared_from_this.hpp>

#include "tcp_connection.h"

using namespace std;

using namespace boost::asio;
using namespace boost::asio::ip;

class tcp_client : public tcp_connection
{
public:
	tcp_client(io_service & io, size_t recv_buff_size = 16 * 1024) : tcp_connection(io, recv_buff_size)
	{}
};

