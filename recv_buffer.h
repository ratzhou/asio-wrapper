
#pragma once

#include <cstddef>

/*
 * 接收缓冲区
 */
class recv_buffer
{
public:
	/*
	 * @size	缓冲区长度
	 */
	recv_buffer(size_t size = 0) : buf(NULL), end(NULL), size(size)
	{
		if (size > 0)
		{
			buf = new char[size];
			end = buf;
		}
	}

	// 禁止拷贝构造
	recv_buffer(const recv_buffer &);

	// move 构造
	recv_buffer(recv_buffer && buffer) : buf(buffer.buf), end(buffer.end), size(buffer.size)
	{
		buffer.buf = NULL;
		buffer.end = NULL;
		buffer.size = 0;
	}

	~recv_buffer()
	{
		delete[] buf;
	}

	// 禁止拷贝赋值
	recv_buffer & operator=(const recv_buffer &);

	// move 赋值
	recv_buffer & operator=(recv_buffer && buffer)
	{
		if (&buffer == this)
			return *this;

		delete[] buf;

		buf = buffer.buf;
		end = buffer.end;
		size = buffer.size;

		buffer.buf = NULL;
		buffer.end = NULL;
		buffer.size = 0;
		return *this;
	}

	// 获取缓冲区地址
	char * get_buff() const
	{
		return end;
	}

	// 获取缓冲区长度
	size_t buff_size() const
	{
		return capacity() - data_size();
	}

	// 获取缓冲区容量
	size_t capacity() const
	{
		return size;
	}

	// 获取数据地址
	const char * get_data() const
	{
		return buf;
	}

	// 获取数据长度
	size_t data_size() const
	{
		return end - buf;
	}

	// 追加数据
	bool append(size_t len)
	{
		if (data_size() + len > capacity())
			return false;
		end += len;
		return true;
	}

	// 消费最多len长度的数据。返回实际消费数据长度
	size_t consume(size_t len)
	{
		size_t has = data_size();
		if (len >= has)
		{
			end = buf;
			return has;
		}
		else
		{
			size_t new_size = has - len;
			memcpy(buf, buf + len, new_size);
			end = buf + new_size;
			return len;
		}
	}

private:
	char * buf;
	char * end;
	size_t size;
};