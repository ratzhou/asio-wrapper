
#pragma once

#include <cstddef>

/*
 * ���ջ�����
 */
class recv_buffer
{
public:
	/*
	 * @size	����������
	 */
	recv_buffer(size_t size = 0) : buf(NULL), end(NULL), size(size)
	{
		if (size > 0)
		{
			buf = new char[size];
			end = buf;
		}
	}

	// ��ֹ��������
	recv_buffer(const recv_buffer &);

	// move ����
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

	// ��ֹ������ֵ
	recv_buffer & operator=(const recv_buffer &);

	// move ��ֵ
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

	// ��ȡ��������ַ
	char * get_buff() const
	{
		return end;
	}

	// ��ȡ����������
	size_t buff_size() const
	{
		return capacity() - data_size();
	}

	// ��ȡ����������
	size_t capacity() const
	{
		return size;
	}

	// ��ȡ���ݵ�ַ
	const char * get_data() const
	{
		return buf;
	}

	// ��ȡ���ݳ���
	size_t data_size() const
	{
		return end - buf;
	}

	// ׷������
	bool append(size_t len)
	{
		if (data_size() + len > capacity())
			return false;
		end += len;
		return true;
	}

	// �������len���ȵ����ݡ�����ʵ���������ݳ���
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