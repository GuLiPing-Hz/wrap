#ifndef DATABLOCK__H__
#define DATABLOCK__H__

#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "pool.h"

/*
	注释添加以及修改于 2014-4-2

	封装一个对数据存储处理的类 DataBlock
	每个函数已经给出简单的释义。
	*/
namespace Wrap {

#define    DEFAULT_BLOCK_SIZE  1024
	/*1024*1024*/
#define    MAX_BLOCK_SIZE  1048576

	class DataBlockBase{
	public:
		DataBlockBase() :m_pos(0), m_size(0){}
		virtual ~DataBlockBase(){}

		/**
		*
		* @param buf 需要拷贝的buf
		* @param buflen buf长度
		* @return 返回当前拷贝进去的字节数； -1表示失败，
		*/
		virtual int append(const char *buf, unsigned int buflen) { return copy(m_pos, buf, buflen); }
		/**
		 *
		 * @param pos 拷贝数据到指定位置
		 * @param buf 需要拷贝的buf
		 * @param buflen buf长度
		 * @return 返回当前拷贝进去的字节数； -1表示失败，
		 */
		virtual int copy(unsigned int pos, const char *buf, unsigned int buflen) = 0;
		/*
		移动buf到DataBlock
		*/
		virtual void move(char *&buf, unsigned int buflen) = 0;

		//获取整个buffer
		virtual const char *getBuf() const = 0;

		//获取当前位置
		virtual unsigned int getPos() const { return m_pos; }

		//清空
		virtual void initPos() { m_pos = 0; }

		//获取大小
		virtual unsigned int getSize() const { return m_size; }
	protected:
		unsigned int m_pos;
		unsigned int m_size;
	};

	/*
	优先推荐使用 DataBlockLocal；
	*/
	class DataBlock : public DataBlockBase {
		DISABLE_COPY_CTOR(DataBlock);
	public:
		DataBlock(unsigned int size = DEFAULT_BLOCK_SIZE) {
			m_pos = 0;
			if (size <= 0) {
				size = 1;
			}

			m_size = size;
			m_buf = (char*)wrap_calloc(m_size);
			//memset(m_buf, 0, m_size);//先置为空
		}

		virtual ~DataBlock() {
			//            LOGD("%s", __FUNCTION__);
			if (m_buf) {
				wrap_free(m_buf);
			}
		}
		//拷贝数据到指定位置
		/*返回当前拷贝进去的字节数*/
		virtual int copy(unsigned int pos, const char *buf, unsigned int buflen) {
			if (!buf || !buflen)
				return 0;

			unsigned int tmppos = pos + buflen;
			//未超出容量
			if (tmppos <= m_size) {
				memcpy(m_buf + pos, buf, buflen);
				m_pos = tmppos;
			}
			else {
				unsigned int newSize = m_size;
				while (newSize < tmppos)
					newSize = newSize << 2;

				char *tmpbuf = (char*)wrap_calloc(newSize);
				if (!tmpbuf)
					return -1;

				//memset(tmpbuf, 0, newSize);//先置为空

				if (m_buf) {
					if (m_pos > 0)
						memcpy(tmpbuf, m_buf, m_pos);
					wrap_free(m_buf);
				}

				memcpy(tmpbuf + pos, buf, buflen);

				m_buf = tmpbuf;
				m_pos = tmppos;
				m_size = newSize;
			}

			return buflen;
		}
		/*
		移动buf到DataBlock
		*/
		virtual void move(char *&buf, unsigned int buflen) {
			if (m_buf) {//释放之前的内容
				wrap_free(m_buf);
			}
			m_buf = buf;
			m_pos = buflen;
			m_size = buflen;

			buf = NULL;//置为空
		}
		//获取整个buffer
		virtual const char *getBuf() const { return m_buf; };

	protected:
		char *m_buf;
	};

	/*
	下面这个Data使用的内部数组的形式，但是构造函数的执行速度是DataBlock 的10倍到20倍之间
	在一些我们确定的buffer大小的情况下，我们可以使用下面的DataBlock作为我们的数据存储对象。

	注意：如果可能存在超越我们指定buffer大小的情况，请使用DataBlock
	*/
	template<unsigned int T = 65535>
	class DataBlockLocal : public DataBlockBase{
		DISABLE_COPY_CTOR(DataBlockLocal);
	public:
		DataBlockLocal(){
			m_size = T;
		}

		virtual int copy(unsigned int pos, const char *buf, unsigned int buflen) {
			if (!buf || !buflen)
				return 0;

			unsigned int tmppos = pos + buflen;
			//未超出容量
			if (tmppos <= m_size) {
				memcpy(m_buf + pos, buf, buflen);
				m_pos = tmppos;
			}
			else {
				return -1;
			}

			return buflen;
		}
		/*
		移动buf到
		*/
		virtual void move(char *&buf, unsigned int buflen) {
			//这里其实非常低效，不建议使用，所以没有实现
		}
		//获取整个buffer
		virtual const char *getBuf() const { return m_buf; };

	protected:
		char m_buf[T];
	};

	typedef DataBlockLocal<> DataBlockLocal65535;
}

#endif//DATABLOCK__H__
