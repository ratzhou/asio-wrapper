
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
	// Ĭ�Ͻ��ջ�����16K
	tcp_connection(io_service & io, size_t recv_buff_size = 16 * 1024) : io(io), so(io), sending(false), recving(false), recv_buff(recv_buff_size)
	{}

	// ��ֹ��������
	tcp_connection(const tcp_connection &);

	virtual ~tcp_connection()
	{}

	// ��ֹ��ֵ
	tcp_connection & operator=(const tcp_connection &);

	io_service & get_io_service()
	{
		return io;
	}

	tcp::socket & get_socket()
	{
		return so;
	}

	// �첽����
	void connect(const string & ip, unsigned short port)
	{
		address addr = address::from_string(ip);
		tcp::endpoint ep(addr, port);
		auto cb = boost::bind(&tcp_connection::handle_connect, shared_from_this(), _1);
		so.open(tcp::v4());
		so.async_connect(ep, cb);
	}

	// �첽����
	void send(const char * data, size_t size)
	{
		send_buff.push_data(data, size);
		if (!sending)
			send_data();
	}

	// �첽����
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
	// tcp_server֪ͨtcp_connection�µ������Ѿ�����
	virtual void start()
	{}

	// tcp_client���첽���ӻص�
	virtual void handle_connect(const boost::system::error_code & ec)
	{}

	// �������ݻص�
	virtual void handle_send(const boost::system::error_code & ec, size_t size)
	{
		sending = false;
		if (!ec)
		{
			sending_buff.consume(size);
			if (!sending_buff.empty())
			{
				// sending_buff��������ݻ�û�з����꣬��sbuff�ϲ���Ȼ����
				send_buff.insert(sending_buff);
			}

			// ���sbuff�����Ƿ������ݵȴ�����
			if (!send_buff.empty())
			{
				// ���������µ����ݵȴ�����
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

	// �������ݻص�
	virtual void handle_recv(const boost::system::error_code & ec, size_t size)
	{
		recving = false;
		if (!ec)
		{
			// ���ճɹ�
			recv_buff.append(size);
			size_t pkg_size = check_package(recv_buff.get_data(), recv_buff.data_size());
			if (pkg_size > 0)
			{
				// ���Ѿ�����
				if (handle_package(recv_buff.get_data(), pkg_size))
				{
					// ���������
					recv_buff.consume(pkg_size);
				}
				else
				{
					// �ر�����
					so.close();
				}
			}

			if (so.is_open())
			{
				// ������������
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
	 * ����������
	 *		@return			�����ȡ�����Ϊ0���������ݰ���û������
	 */
	virtual size_t check_package(const char * buf, size_t size) = 0;

	/*
	 * �������ݰ�
	 *		@return			�Ƿ������������
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

