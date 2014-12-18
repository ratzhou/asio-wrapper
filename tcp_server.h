
#pragma once

#include <string>

#include <boost\asio.hpp>
#include <boost\bind.hpp>
#include <boost\shared_ptr.hpp>
#include <boost\enable_shared_from_this.hpp>

using namespace std;

using namespace boost::asio;
using namespace boost::asio::ip;

/*
 * ServiceType需要满足以下需求
 *		class ServiceType
 *		{
 *		public:
 *			// 构造函数
 *			ServiceType(io_service & io);
 *
 *			// 返回socket对象
 *			tcp::socket & get_socket();
 *
 *			// callback 连接已经建立
 *			void start();
 *		};
 */
template <class ServiceType>
class tcp_server : public boost::enable_shared_from_this<tcp_server<ServiceType> >
{
public:
	typedef tcp_server<ServiceType> this_type;

	tcp_server(io_service & io) : io(io), ac(io)
	{}

	io_service & get_io_service()
	{
		return io;
	}

	// 监听端口
	void start(const string & ip, unsigned short port)
	{
		address addr = address::from_string(ip);
		tcp::endpoint ep(addr, port);
		ac.open(tcp::v4());
		ac.set_option(tcp::socket::reuse_address(true));
		ac.bind(ep);
		ac.listen();
	}

	void accept()
	{
		boost::shared_ptr<ServiceType> service(new ServiceType(io));
		auto cb = boost::bind(&this_type::handle_accept, shared_from_this(), service, _1);
		ac.async_accept(service->get_socket(), cb);
	}

protected:
	void handle_accept(boost::shared_ptr<ServiceType> & service, const boost::system::error_code & ec)
	{
		if (!ec)
		{
			// accept成功，继续发起异步accept
			try
			{
				accept();
			}
			catch (std::exception &)
			{
				// 异常
				return;
			}

			// 通知Service对象连接已经建立
			service->start();
		}
		else if (ec == error::operation_aborted)
		{
			// 取消了
			ac.close();
		}
		else
		{
			// 错误
			cerr << "acceptor error: " << ec.message() << endl;
		}
	}

private:
	io_service & io;
	tcp::acceptor ac;
};

