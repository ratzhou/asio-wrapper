
#pragma once

#include <deque>
#include <vector>
#include <utility>

#include <boost\asio.hpp>

using namespace std;
using namespace boost::asio;

/*
 * ���ͻ�����
 */
class send_buffer
{
public:
	// ��Ϣ��
	class message
	{
	public:
		message() : buf(NULL), cur(NULL), size(0)
		{}

		// ������Ϣ�塣��ȿ���
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

		// ��ֹ��������
		message(const message & msg);

		// move ����
		message(message && msg)
		{
			buf = msg.buf;
			cur = msg.cur;
			size = msg.size;

			msg.buf = NULL;
			msg.cur = NULL;
			msg.size = 0;
		}

		// �ͷ��ڴ�
		~message()
		{
			delete[] buf;
		}

		// ��ֹ������ֵ
		message & operator=(const message &);

		// move ��ֵ
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

		// ��ȡ���ݵ�ַ
		const char * get_data() const
		{
			return cur;
		}

		// ��ȡ���ݳ���
		size_t data_size() const
		{
			return size - (cur - buf);
		}

		// �������len���ȵ����ݡ�����ʵ�����ѵ����ݳ���
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

		// ת��Ϊconst_buffer����
		operator const_buffer() const
		{
			return const_buffer(cur, data_size());
		}

	private:
		char * buf;
		char * cur;
		size_t size;
	};


	// ���췢�ͻ�����
	send_buffer()
	{}

	// ��ֹ��������
	send_buffer(const send_buffer &);

	// move ����
	send_buffer(send_buffer && buf) : msg_deque(std::move(buf.msg_deque))
	{}

	// ��ֹ������ֵ
	send_buffer & operator=(const send_buffer &);

	// move��ֵ
	send_buffer & operator=(send_buffer && buf)
	{
		msg_deque = std::move(buf.msg_deque);
		return *this;
	}

	// ׷�ӷ�������
	void push_data(const char * buf, size_t size)
	{
		if (size > 0)
		{
			message msg(buf, size);
			msg_deque.push_back(std::move(msg));
		}
	}

	// ���뷢������
	void insert(send_buffer & oth)
	{
		for (auto rit = oth.msg_deque.rbegin(); rit != oth.msg_deque.rend(); ++rit)
			msg_deque.push_front(std::move(*rit));
		oth.clear();
	}

	// ��ȡ��������
	vector<const_buffer> get_buffers() const
	{
		vector<const_buffer> buffers;
		buffers.reserve(msg_deque.size());
		for (auto it = msg_deque.begin(); it != msg_deque.end(); ++it)
			buffers.push_back(*it);
		return buffers;
	}

	// ��������
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
				// ���������˾͵���
				++it;
				msg_deque.pop_front();
			}
			else
			{
				// ��������û�����꣬��ʱ��len�϶�����0
				++it;
			}
		}
		return total_consumed;
	}

	// ���ݳ���
	size_t data_size() const
	{
		size_t size = 0;
		for (auto it = msg_deque.begin(); it != msg_deque.end(); ++it)
			size += it->data_size();
		return size;
	}

	// �������
	void clear()
	{
		msg_deque.clear();
	}

	// �Ƿ�ջ�����
	bool empty() const
	{
		return data_size() == 0;
	}

private:
	deque<message> msg_deque;
};
