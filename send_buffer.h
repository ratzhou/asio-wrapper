
#pragma once

#include <deque>
#include <vector>
#include <utility>

#include <boost\asio.hpp>

using namespace std;
using namespace boost::asio;

/*
 * 发送缓冲区
 */
class send_buffer
{
public:
	// 消息体
	class message
	{
	public:
		message() : buf(NULL), cur(NULL), size(0)
		{}

		// 构造消息体。深度拷贝
		message(const char * buf, size_t size)
		{
			if (buf != NULL && size > 0)
			{
				this->buf = new char[size];
				memcpy(this->buf, buf, size);
				this->cur = this->buf;
				this->size = size;
			}
		}

		// 禁止拷贝构造
		message(const message & msg);

		// move 构造
		message(message && msg)
		{
			buf = msg.buf;
			cur = msg.cur;
			size = msg.size;

			msg.buf = NULL;
			msg.cur = NULL;
			msg.size = 0;
		}

		// 释放内存
		~message()
		{
			delete[] buf;
		}

		// 禁止拷贝赋值
		message & operator=(const message &);

		// move 赋值
		message & operator=(message && msg)
		{
			if (&msg == this)
				return *this;

			delete[] buf;

			buf = msg.buf;
			cur = msg.cur;
			size = msg.size;

			msg.buf = NULL;
			msg.cur = NULL;
			msg.size = 0;
			return *this;
		}

		// 获取数据地址
		const char * get_data() const
		{
			return cur;
		}

		// 获取数据长度
		size_t data_size() const
		{
			return size - (cur - buf);
		}

		// 消费最多len长度的数据。返回实际消费的数据长度
		size_t consume(size_t len)
		{
			size_t has = data_size();
			if (len >= has)
			{
				cur = buf + size;
				return has;
			}
			else
			{
				cur = cur + len;
				return len;
			}
		}

		// 转化为const_buffer对象
		operator const_buffer() const
		{
			return const_buffer(cur, data_size());
		}

	private:
		char * buf;
		char * cur;
		size_t size;
	};


	// 构造发送缓冲区
	send_buffer()
	{}

	// 禁止拷贝构造
	send_buffer(const send_buffer &);

	// move 构造
	send_buffer(send_buffer && buf) : msg_deque(std::move(buf.msg_deque))
	{}

	// 禁止拷贝赋值
	send_buffer & operator=(const send_buffer &);

	// move赋值
	send_buffer & operator=(send_buffer && buf)
	{
		msg_deque = std::move(buf.msg_deque);
		return *this;
	}

	// 追加发送数据
	void push_data(const char * buf, size_t size)
	{
		if (size > 0)
		{
			message msg(buf, size);
			msg_deque.push_back(std::move(msg));
		}
	}

	// 插入发送数据
	void insert(send_buffer & oth)
	{
		for (auto rit = oth.msg_deque.rbegin(); rit != oth.msg_deque.rend(); ++rit)
			msg_deque.push_front(std::move(*rit));
		oth.clear();
	}

	// 获取发送数据
	vector<const_buffer> get_buffers() const
	{
		vector<const_buffer> buffers;
		buffers.reserve(msg_deque.size());
		for (auto it = msg_deque.begin(); it != msg_deque.end(); ++it)
			buffers.push_back(*it);
		return buffers;
	}

	// 消费数据
	size_t consume(size_t len)
	{
		size_t total_consumed = 0;
		auto it = msg_deque.begin();
		while (len > 0 && it != msg_deque.end())
		{
			size_t consumed = it->consume(len);
			len -= consumed;
			total_consumed += consumed;

			if (it->data_size() == 0)
			{
				// 数据用完了就弹出
				++it;
				msg_deque.pop_front();
			}
			else
			{
				// 还有数据没消费完，这时候len肯定等于0
				++it;
			}
		}
		return total_consumed;
	}

	// 数据长度
	size_t data_size() const
	{
		size_t size = 0;
		for (auto it = msg_deque.begin(); it != msg_deque.end(); ++it)
			size += it->data_size();
		return size;
	}

	// 清空数据
	void clear()
	{
		msg_deque.clear();
	}

	// 是否空缓冲区
	bool empty() const
	{
		return data_size() == 0;
	}

private:
	deque<message> msg_deque;
};
