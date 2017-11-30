#ifndef NATIVE_BUFFER_H___
#define NATIVE_BUFFER_H___

#include <string>
#include <vector>
#include "data_block.h"
#include "funcs.h"
#include "config.h"

#ifdef COCOS_PROJECT
#include "cocos2d.h"
#include "jsapi.h"
#include "jsfriendapi.h"
#include "scripting/js-bindings/manual/cocos2d_specifics.hpp"

std::string jsval_to_std_string_len(JSContext *cx, JS::HandleValue arg);
void register_NativeBuffer(JSContext *cx, JS::HandleObject ns);

#endif

typedef std::vector<std::string> VECSTRING;

namespace Wrap{

	class BufferValue;

	/*
	大端buffer
	*/
	class NativeBuffer
#ifdef COCOS_PROJECT
		: public cocos2d::Ref
#endif
	{
#ifdef COCOS_PROJECT
	protected:
#else
	public:
#endif
		NativeBuffer(char format = OP_BIGENDIAN);
		virtual ~NativeBuffer();

#ifdef COCOS_PROJECT
	public:
		static NativeBuffer *Create();
#endif
		//读写位置归0
		void clearBuffer();

		void moveBuffer(char *&data, unsigned int len);

		const DataBlockLocal65535* getBuffer() const;

		bool hasData();

		bool skipBuffer(const unsigned int len);

		/**
		*
		* @param c
		* @return 是否写入成功
		*/
		template<typename T>
		bool writeType(const T c);

		template<typename T>
		bool readType(T &c);

		bool writeChar(const char c);

		bool writeUChar(const unsigned char c);

		bool writeShort(const short c);

		bool writeUShort(const unsigned short c);

		bool writeInt(const int c);

		bool writeUInt(const unsigned int c);

		bool writeInt64(const long long c);

		bool writeUInt64(const unsigned long long c);

		bool writeFloat(const float c);

		bool writeString(const unsigned short len, const char *c);

		bool writeStringNoLen(const unsigned short len, const char *c);

		bool readChar(char &c);

		bool readUChar(unsigned char &c);

		bool readShort(short &c);

		bool readUShort(unsigned short &c);

		bool readInt(int &c);

		bool readUInt(unsigned int &c);

		bool readInt64(long long &c);

		bool readUInt64(unsigned long long &c);

		bool readFloat(float &c);

		bool readString(unsigned short &len, char *c);

		bool readBuffer(char *c, unsigned int len);

		std::string readString();

	private:
		DataBlockLocal65535 data_;
		bool swap_;

		//记录读取位置
		unsigned int readPos_;
	};

	//自动解析网络的协议数据
	BufferValue* AutoParseNativeBufferEx(NativeBuffer *nativeBuf);

}

#endif//NATIVE_BUFFER_H___
