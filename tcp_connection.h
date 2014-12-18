
#pragma once

#include <string>

#include <boost\asio.hpp>
#include <boost\bind.hpp>
#include <boost\shared_ptr.hpp>
#include <boost\enable_shared_from_this.hpp>

#include "recv_buffer.h"
#include "send_buffer.h"

using namespace std;

using namespace boost::asio;
using namespace boost::asio::ip;

class tcp_connection : public boost::enable_shared_from_this<tcp_connection>
{
public:
	// 默认接收缓冲区16K
	tcp_connection(io_service & io, size_t recv_buff_size = 16 * 1024) : io(io), so(io), sending(false), recving(false), recv_buff(recv_buff_size)
	{}

	// 禁止拷贝构造
	tcp_connection(const tcp_connection &);

	virtual ~tcp_connection()
	{}

	// 禁止赋值
	tcp_connection & operator=(const tcp_connection &);

	io_service & get_io_service()
	{
		return io;
	}

	tcp::socket & get_socket()
	{
		return so;
	}

	// 异步连接
	void connect(const string & ip, unsigned short port)
	{
		address addr = address::from_string(ip);
		tcp::endpoint ep(addr, port);
		auto cb = boost::bind(&tcp_connection::handle_connect, shared_from_this(), _1);
		so.open(tcp::v4());
		so.async_connect(ep, cb);
	}

	// 异步发送
	void send(const char * data, size_t size)
	{
		send_buff.push_data(data, size);
		if (!sending)
			send_data();
	}

	// 异步接收
	void recv()
	{
		if (recving)
			return;

		auto buffers = mutable_buffers_1(recv_buff.get_buff(), recv_buff.buff_size());
		auto cb = boost::bind(&tcp_connection::handle_recv, shared_from_this(), _1, _2);
		so.async_receive(buffers, cb);
		recving = true;
	}

	void close()
	{
		so.close();
	}

protected:
	// tcp_server通知tcp_connection新的连接已经建立
	virtual void start()
	{}

	// tcp_client的异步连接回调
	virtual void handle_connect(const boost::system::error_code & ec)
	{}

	// 发送数据回调
	virtual void handle_send(const boost::system::error_code & ec, size_t size)
	{
		sending = false;
		if (!ec)
		{
			sending_buff.consume(size);
			if (!sending_buff.empty())
			{
				// sending_buff里面的数据还没有发送完，与sbuff合并，然后发送
				send_buff.insert(sending_buff);
			}

			// 检查sbuff里面是否有数据等待发送
			if (!send_buff.empty())
			{
				// 缓冲区有新的数据等待发送
				try
				{
					send_data();
				}
				catch (std::exception &)
				{
					so.close();
				}
			}
		}
	}

	// 接收数据回调
	virtual void handle_recv(const boost::system::error_code & ec, size_t size)
	{
		recving = false;
		if (!ec)
		{
			// 接收成功
			recv_buff.append(size);
			size_t pkg_size = check_package(recv_buff.get_data(), recv_buff.data_size());
			if (pkg_size > 0)
			{
				// 包已经完整
				if (handle_package(recv_buff.get_data(), pkg_size))
				{
					// 抽出包数据
					recv_buff.consume(pkg_size);
				}
				else
				{
					// 关闭连接
					so.close();
				}
			}

			if (so.is_open())
			{
				// 继续接收数据
				try
				{
					recv();
				}
				catch (std::exception &)
				{
					so.close();
				}
			}
		}
	}

	void send_data()
	{
		sending_buff = std::move(send_buff);
		vector<const_buffer> buffers = sending_buff.get_buffers();
		auto cb = boost::bind(&tcp_connection::handle_send, shared_from_this(), _1, _2);
		async_write(so, buffers, cb);
		sending = true;
	}

	/*
	 * 检测包完整性
	 *		@return			包长度。假如为0，代表数据包还没有完整
	 */
	virtual size_t check_package(const char * buf, size_t size) = 0;

	/*
	 * 处理数据包
	 *		@return			是否继续接收数据
	 */
	virtual bool handle_package(const char * buf, size_t size) = 0;

private:
	io_service & io;
	tcp::socket so;
	bool sending;
	bool recving;
	recv_buffer recv_buff;
	send_buffer send_buff;
	send_buffer sending_buff;
};

