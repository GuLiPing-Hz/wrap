#include "native_buffer.h"
#include <memory>
#include "buffer_value.h"
#include "pool.h"

namespace Wrap{

	BufferValue *ReadNativeBufferValue(NativeBuffer *nativeBuf, int type, int &len) {
		wrap_new_begin;
		BufferValue *ret = wrap_new(BufferValue);//PoolMgr::GetIns()->getFromPool<BufferValue>("BufferValue");
		if (!ret)
			return ret;

		ret->type = (BufferValue::eDataType) type;

		if (type == BufferValue::type_char) {
			nativeBuf->readChar(ret->data.base.c);
			len = 1;
		}
		else if (type == BufferValue::type_short) {
			nativeBuf->readShort(ret->data.base.s);
			len = 2;
		}
		else if (type == BufferValue::type_int) {
			nativeBuf->readInt(ret->data.base.i);
			len = 4;
		}
		else if (type == BufferValue::type_int64) {
			//这里 js引擎默认把long long 型数据转换为stirng，所以需要数字比较的时候记得转int
			nativeBuf->readInt64(ret->data.base.ll);//直接我这里把它转了
			len = 8;
		}
		else if (type == BufferValue::type_float) {
			nativeBuf->readFloat(ret->data.base.f);
			len = 4;
		}
		else if (type == BufferValue::type_str) {
			ret->data.str = nativeBuf->readString();
			len = 2 + (int)ret->data.str.length();//字符串长度+字符串
		}
		else {
			ret->type = BufferValue::type_unknow;
			LOGE("readNativeBufferSingle unknown type= %d", type);
		}

		// Log.i("readNativeBufferSingle type=" + typeof ret.value + ",value = " + ret.value);
		return ret;
	}

	BufferValue* AutoParseNativeBufferEx(NativeBuffer *nativeBuf){
		BufferValue* ret = NULL;
		if (!nativeBuf)
			return NULL;

		wrap_new_begin;
		ret = wrap_new(BufferValue);//PoolMgr::GetIns()->getFromPool<BufferValue>("BufferValue");
		do {
			char type;
			nativeBuf->readChar(type);//读取标志位

			if (type > 22)//异常type
				break;

			if (type > BufferValue::type_array + 1) {//字符串解析不能放在这里
				char realType = type & 0xf;//数据具体类型
				short arraLen;
				nativeBuf->readShort(arraLen);
				// Log.i("readNativeBufferData realType=" + realType + ",arraLen=" + arraLen);

				BufferValue *arra = wrap_new(BufferValue);//PoolMgr::GetIns()->getFromPool<BufferValue>("BufferValue");//数组存储
				arra->type = (BufferValue::eDataType) realType;//数组的具体类型
				if (realType == 6) {//自定义结构数据
					//arra->isInner = true;
					if (arraLen > 0) {//不是空数组
						for (int i = 0; i < arraLen; i++) {
							//读取数据结构长度
							short structLen;
							nativeBuf->readShort(structLen);
							// Log.i("readNativeBufferData structLen = " + structLen);

							int tempLen = 0;
							BufferValue *struc = wrap_new(BufferValue);//PoolMgr::GetIns()->getFromPool<BufferValue>("BufferValue");//数据结构
							
							//struc->isInner = false;
							struc->type = BufferValue::type_custom;
							while (tempLen < structLen) {
								char tempType;
								nativeBuf->readChar(tempType);
								tempLen += 1;//1字节

								int inLen = 0;
								BufferValue *tempRet = ReadNativeBufferValue(nativeBuf, tempType, inLen);
								struc->list.push_back(tempRet);//把值记录下来

								if (inLen == 0) {//出现异常了！！！，
									int remainLen = structLen - tempLen;
									if (remainLen > 0) {
										//把多余的数据跳过一下
										nativeBuf->skipBuffer(remainLen);
									}
									break;
								}

								tempLen += inLen;
							}
							arra->list.push_back(struc);//放入结构数据
						}
					}
				}
				else {
					//arra->isInner = false;
					for (int i = 0; i < arraLen; i++) {
						int inLen = 0;
						BufferValue *tempRet = ReadNativeBufferValue(nativeBuf, realType, inLen);
						arra->list.push_back(tempRet);//把值记录下来
					}
				}
				ret->list.push_back(arra);
			}
			else {
				int inLen = 0;
				BufferValue *tempRet = ReadNativeBufferValue(nativeBuf, type, inLen);
				ret->list.push_back(tempRet);//把值记录下来
			}

		} while (nativeBuf->hasData());

		return ret;
	}

#ifdef COCOS_PROJECT
	NativeBuffer *NativeBuffer::Create() {
		NativeBuffer *buf = new NativeBuffer();
		if (buf)
			buf->autorelease();
		return buf;
	}
#endif

	NativeBuffer::NativeBuffer(char format)
		:
#ifdef COCOS_PROJECT
		cocos2d::Ref(),
#endif
		readPos_(0) {
		swap_ = doendian(format);
	}

	NativeBuffer::~NativeBuffer() {
		//    LOGD("%s", __FUNCTION__);
		//int glp = 1;
	}

	//读写位置归0
	void NativeBuffer::clearBuffer() {
		readPos_ = 0;
		data_.initPos();
	}

	void NativeBuffer::moveBuffer(char *&data, unsigned int len) {
		readPos_ = 0;
		data_.move(data, len);
	}

	const DataBlockLocal65535* NativeBuffer::getBuffer() const{
		return &data_;
	}

	bool NativeBuffer::hasData() {
		return readPos_ < data_.getPos();
	}

	bool NativeBuffer::skipBuffer(const unsigned int len) {
		if (readPos_ + len < data_.getPos()) {
			readPos_ = readPos_ + len;
			return true;
		}
		return false;
	}

	bool NativeBuffer::writeChar(const char c) {
		return data_.append(&c, sizeof(c)) > 0;
	}

	bool NativeBuffer::writeUChar(const unsigned char c){
		return data_.append((char*)&c, sizeof(c)) > 0;
	}

	bool NativeBuffer::writeShort(const short c) {
		return writeType(c);
	}

	bool NativeBuffer::writeUShort(const unsigned short c){
		return writeType(c);
	}

	bool NativeBuffer::writeInt(const int c) {
		return writeType(c);
	}

	bool NativeBuffer::writeUInt(const unsigned int c){
		return writeType(c);
	}

	bool NativeBuffer::writeInt64(const long long c) {
		return writeType(c);
	}

	bool NativeBuffer::writeUInt64(const unsigned long long c){
		return writeType(c);
	}

	bool NativeBuffer::writeFloat(const float c) {
		return writeType(c);
	}

	bool NativeBuffer::writeString(const unsigned short len, const char *c) {
		if (writeType(len))//写入字符串长度
			return data_.append(c, len) >= 0;//写入字符串  这里可以写入一个空的字符串
		else
			return false;
	}

	bool NativeBuffer::writeStringNoLen(const unsigned short len, const char *c) {
		return data_.append(c, len) > 0;//写入字符串
	}

	bool NativeBuffer::readBuffer(char *c, unsigned int len) {
		if (readPos_ + len > data_.getPos())//越界了
			return false;

		memcpy(c, data_.getBuf() + readPos_, len);
		readPos_ += len;
		return true;
	}

	bool NativeBuffer::readChar(char &c) {
		return readBuffer(&c, sizeof(c));
	}

	bool NativeBuffer::readUChar(unsigned char &c){
		return readBuffer((char*)&c, sizeof(c));
	}

	bool NativeBuffer::readShort(short &c) {
		return readType(c);
	}

	bool NativeBuffer::readUShort(unsigned short &c){
		return readType(c);
	}

	bool NativeBuffer::readInt(int &c) {
		return readType(c);
	}

	bool NativeBuffer::readUInt(unsigned int &c){
		return readType(c);
	}

	bool NativeBuffer::readInt64(long long &c) {
		return readType(c);
	}

	bool NativeBuffer::readUInt64(unsigned long long &c){
		return readType(c);
	}

	bool NativeBuffer::readFloat(float &c) {
		return readType(c);
	}

	bool NativeBuffer::readString(unsigned short &len, char *c) {
		bool ret = readType(len);
		if (c && ret)
			return readBuffer(c, len);
		else {
			readPos_ -= 2;//把2字节还回去
			return ret;
		}
	}

	template<typename T>
	bool NativeBuffer::writeType(T c) {
		int size = sizeof(c);
		doswap(swap_, (char *)&c, size);
		return data_.append((const char *)&c, size) > 0;
	}

	template<typename T>
	bool NativeBuffer::readType(T &c) {
		bool ret = readBuffer((char *)&c, sizeof(c));
		if (ret) {
			doswap(swap_, &c, sizeof(c));
			return true;
		}
		return false;
	}

	std::string NativeBuffer::readString() {
		unsigned short c = 0;
		readString(c, nullptr);//读取字符串长度

		std::string ret;
		char* p = (char*)wrap_calloc(c);
		if (p){
			//std::unique_ptr<char> pAuto(p);//自动指针
			Wrap::VoidGuard guard(p);
			std::string str;
			if (readString(c, p)) {//读取字符串
				std::string temp(p, c);
				ret = std::move(temp);
			}
		}

		return ret;
	}

}

#ifdef COCOS_PROJECT

std::string jsval_to_std_string_len(JSContext *cx, JS::HandleValue arg){
	if (!arg.isString())
		return "";

	std::string ret;
	size_t len = JS_GetStringEncodingLength(cx, arg.toString());
	if (len != -1 && len > 0){
		char* temp = (char*)wrap_calloc(len);
		if (temp){
			//std::unique_ptr<char> pAuto(temp);//自动指针
			Wrap::VoidGuard guard(temp);
			JS_EncodeStringToBuffer(cx, arg.toString(), temp, len);
			std::string tempStr(temp, len);
			ret = std::move(tempStr);
		}
	}
	return ret;
}

jsval utf8string_to_jsval(JSContext *cx, const std::string& v){
	JS::RootedObject tmp(cx, JS_NewObject(cx, NULL, JS::NullPtr(), JS::NullPtr()));
	if (!tmp) return JSVAL_NULL;

	jsval jsstr = std_string_to_jsval(cx, v);
	JS::RootedString a(cx, jsstr.toString());
	bool ok = JS_DefineProperty(cx, tmp, "len", (uint32_t)v.size(), JSPROP_ENUMERATE | JSPROP_PERMANENT) &&
		JS_DefineProperty(cx, tmp, "str", a, JSPROP_ENUMERATE | JSPROP_PERMANENT);
	if (ok) {
		return OBJECT_TO_JSVAL(tmp);
	}
	return JSVAL_NULL;
}

bool JS_Native_writeChar(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_writeChar : Invalid Native Object");
	if (argc == 1)
	{
		char c;
		int temp = 0;
		ok &= jsval_to_int(cx, args.get(0), &temp);
		JSB_PRECONDITION2(ok, cx, false, "JS_Native_writeChar : Error processing method arguments");
		c = (char)temp;

		bool ret = cobj->writeChar(c);

		JS::RootedValue jsret(cx);
		jsret = BOOLEAN_TO_JSVAL(ret);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_writeChar : wrong number of arguments: %d, was expecting %d", argc, 1);
	return false;
}

bool JS_Native_writeShort(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_writeShort : Invalid Native Object");
	if (argc == 1)
	{
		short c;
		int temp = 0;
		ok &= jsval_to_int(cx, args.get(0), &temp);
		JSB_PRECONDITION2(ok, cx, false, "JS_Native_writeShort : Error processing method arguments");
		c = (short)temp;

		bool ret = cobj->writeShort(c);

		JS::RootedValue jsret(cx);
		jsret = BOOLEAN_TO_JSVAL(ret);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_writeShort : wrong number of arguments: %d, was expecting %d", argc, 1);
	return false;
}

bool JS_Native_writeInt(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_writeInt : Invalid Native Object");
	if (argc == 1)
	{
		int c;
		int temp = 0;
		ok &= jsval_to_int(cx, args.get(0), &temp);
		JSB_PRECONDITION2(ok, cx, false, "JS_Native_writeInt : Error processing method arguments");
		c = temp;

		bool ret = cobj->writeInt(c);

		JS::RootedValue jsret(cx);
		jsret = BOOLEAN_TO_JSVAL(ret);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_writeInt : wrong number of arguments: %d, was expecting %d", argc, 1);
	return false;
}

bool JS_Native_writeInt64(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_writeInt64 : Invalid Native Object");
	if (argc == 1)
	{
		long long c;
		ok &= jsval_to_long_long(cx, args.get(0), &c);
		JSB_PRECONDITION2(ok, cx, false, "JS_Native_writeInt64 : Error processing method arguments");

		bool ret = cobj->writeInt64(c);

		JS::RootedValue jsret(cx);
		jsret = BOOLEAN_TO_JSVAL(ret);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_writeInt64 : wrong number of arguments: %d, was expecting %d", argc, 1);
	return false;
}

bool JS_Native_writeFloat(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_writeFloat : Invalid Native Object");
	if (argc == 1)
	{
		float c;

		double dp;
		ok &= JS::ToNumber(cx, args.get(0), &dp);
		JSB_PRECONDITION2(ok, cx, false, "JS_Native_writeFloat : Error processing method arguments");
		ok &= !std::isnan(dp);
		JSB_PRECONDITION2(ok, cx, false, "JS_Native_writeFloat : Error processing method arguments");
		c = (float)dp;

		bool ret = cobj->writeFloat(c);

		JS::RootedValue jsret(cx);
		jsret = BOOLEAN_TO_JSVAL(ret);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_writeFloat : wrong number of arguments: %d, was expecting %d", argc, 1);
	return false;
}

bool JS_Native_writeString(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_writeString : Invalid Native Object");
	if (argc == 1)
	{
		std::string str = jsval_to_std_string_len(cx, args.get(0));
		bool ret = cobj->writeString(str.size(), str.c_str());

		std::string str2;
		jsval_to_std_string(cx, args.get(0), &str2);

		JS::RootedValue jsret(cx);
		jsret = BOOLEAN_TO_JSVAL(ret);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_writeString : wrong number of arguments: %d, was expecting %d", argc, 1);
	return false;
}

bool JS_Native_writeStringNoLen(JSContext *cx, unsigned int argc, jsval *vp){
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_writeString : Invalid Native Object");
	if (argc == 1)
	{
		std::string str = jsval_to_std_string_len(cx, args.get(0));
		bool ret = cobj->writeStringNoLen(str.size(), str.c_str());

		JS::RootedValue jsret(cx);
		jsret = BOOLEAN_TO_JSVAL(ret);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_writeString : wrong number of arguments: %d, was expecting %d", argc, 1);
	return false;
}

bool JS_Native_writeStringWithUtf8(JSContext *cx, unsigned int argc, jsval *vp){
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_writeStringWithUtf8 : Invalid Native Object");
	if (argc == 1)
	{
		std::string str;
		jsval_to_std_string(cx, args.get(0), &str);
		bool ret = cobj->writeString(str.size(), str.c_str());

		JS::RootedValue jsret(cx);
		jsret = BOOLEAN_TO_JSVAL(ret);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_writeStringWithUtf8 : wrong number of arguments: %d, was expecting %d", argc, 1);
	return false;
}

bool JS_Native_getBuffer(JSContext *cx, unsigned int argc, jsval *vp){
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_getBuffer : Invalid Native Object");
	if (argc == 0)
	{
		const Wrap::DataBlockLocal65535* block = cobj->getBuffer();

		JS::RootedValue jsret(cx);
		jsret = STRING_TO_JSVAL(JS_NewStringCopyN(cx, block->getBuf(), block->getPos()));
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_getBuffer : wrong number of arguments: %d, was expecting %d", argc, 0);
	return false;
}

bool JS_Native_getBufferLen(JSContext *cx, unsigned int argc, jsval *vp){
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_getBufferLen : Invalid Native Object");
	if (argc == 0)
	{
		unsigned int len = cobj->getBuffer()->getPos();

		JS::RootedValue jsret(cx);
		jsret = UINT_TO_JSVAL(len);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_getBufferLen : wrong number of arguments: %d, was expecting %d", argc, 0);
	return false;
}

bool JS_Native_clearBuffer(JSContext *cx, unsigned int argc, jsval *vp){
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_clearBuffer : Invalid Native Object");
	if (argc == 0)
	{
		cobj->clearBuffer();//清空buffer
		args.rval().setUndefined();

		return true;
	}

	JS_ReportError(cx, "JS_Native_clearBuffer : wrong number of arguments: %d, was expecting %d", argc, 0);
	return false;
}

bool JS_Native_readChar(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_readChar : Invalid Native Object");
	if (argc == 0)
	{
		char c = 0;
		cobj->readChar(c);
		int temp = c;

		JS::RootedValue jsret(cx);
		jsret = INT_TO_JSVAL(temp);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_readChar : wrong number of arguments: %d, was expecting %d", argc, 0);
	return false;
}

bool JS_Native_readShort(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_readShort : Invalid Native Object");
	if (argc == 0)
	{
		short c = 0;
		cobj->readShort(c);
		int temp = c;

		JS::RootedValue jsret(cx);
		jsret = INT_TO_JSVAL(temp);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_readShort : wrong number of arguments: %d, was expecting %d", argc, 0);
	return false;
}

bool JS_Native_readInt(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_readInt : Invalid Native Object");
	if (argc == 0)
	{
		int c = 0;
		cobj->readInt(c);
		int temp = c;

		JS::RootedValue jsret(cx);
		jsret = INT_TO_JSVAL(temp);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_readInt : wrong number of arguments: %d, was expecting %d", argc, 0);
	return false;
}

bool JS_Native_readInt64(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_readInt64 : Invalid Native Object");
	if (argc == 0)
	{
		long long c = 0L;
		cobj->readInt64(c);

		JS::RootedValue jsret(cx);
		jsret = long_long_to_jsval(cx, c);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_readInt64 : wrong number of arguments: %d, was expecting %d", argc, 0);
	return false;
}

bool JS_Native_readFloat(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_readFloat : Invalid Native Object");
	if (argc == 0)
	{
		float c = 0.0f;
		cobj->readFloat(c);

		JS::RootedValue jsret(cx);
		jsret = DOUBLE_TO_JSVAL(c);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_readFloat : wrong number of arguments: %d, was expecting %d", argc, 0);
	return false;
}

bool JS_Native_readString(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_readString : Invalid Native Object");
	if (argc == 0)
	{
		unsigned short c = 0;
		cobj->readString(c, nullptr);//读取字符串长度

		char* p = (char*)wrap_calloc(c);
		if (p){
			//std::unique_ptr<char> pAuto(p);//自动指针
			Wrap::VoidGuard guard(p);
			std::string str;
			if (cobj->readString(c, p)){//读取字符串
				std::string temp(p, c);
				str = std::move(temp);
			}
			else{
				JS_ReportError(cx, "JS_Native_readString : readString failed");
				return false;
			}

			JS::RootedValue jsret(cx);
			jsret = STRING_TO_JSVAL(JS_NewStringCopyN(cx, str.c_str(), str.size()));// std_string_to_jsval(cx, str);
			args.rval().set(jsret);

			return true;
		}
		else{
			JS_ReportError(cx, "JS_Native_readString : oom");
			return false;
		}
	}

	JS_ReportError(cx, "JS_Native_readString : wrong number of arguments: %d, was expecting %d", argc, 0);
	return false;
}

bool JS_Native_readStringNoLen(JSContext *cx, unsigned int argc, jsval *vp){
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_readStringNoLen : Invalid Native Object");
	if (argc == 1)
	{
		unsigned int len = 0;
		ok &= jsval_to_uint32(cx, args.get(0), &len);
		JSB_PRECONDITION2(ok, cx, false, "JS_Native_readStringNoLen : Error processing method arguments");

		char* p = (char*)wrap_calloc(len);
		if (p){
			//std::unique_ptr<char> pAuto(p);//自动指针
			Wrap::VoidGuard guard(p);
			std::string str;
			if (cobj->readBuffer(p, len)){//读取字符串
				std::string temp(p, len);
				str = std::move(temp);
			}
			else{
				JS_ReportError(cx, "JS_Native_readStringNoLen : readBuffer Error");
				return false;
			}

			JS::RootedValue jsret(cx);
			jsret = STRING_TO_JSVAL(JS_NewStringCopyN(cx, str.c_str(), str.size()));
			args.rval().set(jsret);

			return true;
		}
		else{
			JS_ReportError(cx, "JS_Native_readStringNoLen : oom");
			return false;
		}
	}

	JS_ReportError(cx, "JS_Native_readStringNoLen : wrong number of arguments: %d, was expecting %d", argc, 0);
	return false;
}

bool JS_Native_readStringWithUtf8(JSContext *cx, unsigned int argc, jsval *vp){
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_readStringWithUtf8 : Invalid Native Object");
	if (argc == 0)
	{
		unsigned short c = 0;
		cobj->readString(c, nullptr);//读取字符串长度

		char* p = (char*)wrap_calloc(c);
		if (p){
			//std::unique_ptr<char> pAuto(p);//自动指针
			Wrap::VoidGuard guard(p);
			std::string str;
			if (cobj->readString(c, p)){//读取字符串
				std::string temp(p, c);
				str = std::move(temp);
			}
			else{
				JS_ReportError(cx, "JS_Native_readStringWithUtf8 : readString failed");
				return false;
			}

			JS::RootedValue jsret(cx);
			jsret = utf8string_to_jsval(cx, str);
			args.rval().set(jsret);

			return true;
		}
		else{
			JS_ReportError(cx, "JS_Native_readStringWithUtf8 : oom");
			return false;
		}
	}

	JS_ReportError(cx, "JS_Native_readStringWithUtf8 : wrong number of arguments: %d, was expecting %d", argc, 0);
	return false;
}

bool JS_Native_hasData(JSContext *cx, unsigned int argc, jsval *vp){
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_hasData : Invalid Native Object");
	if (argc == 0)
	{
		bool ret = cobj->hasData();

		JS::RootedValue jsret(cx);
		jsret = BOOLEAN_TO_JSVAL(ret);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_hasData : wrong number of arguments: %d, was expecting %d", argc, 0);
	return false;
}

bool JS_Native_skipBuffer(JSContext *cx, unsigned int argc, jsval *vp){
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	bool ok = true;
	JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
	js_proxy_t *proxy = jsb_get_js_proxy(obj);
	Wrap::NativeBuffer *cobj = (Wrap::NativeBuffer *)(proxy ? proxy->ptr : NULL);
	JSB_PRECONDITION2(cobj, cx, false, "JS_Native_skipBuffer : Invalid Native Object");
	if (argc == 1)
	{
		unsigned int len;
		ok &= jsval_to_uint32(cx, args.get(0), &len);
		JSB_PRECONDITION2(ok, cx, false, "JS_Native_skipBuffer : Error processing method arguments");

		bool ret = cobj->skipBuffer(len);

		JS::RootedValue jsret(cx);
		jsret = BOOLEAN_TO_JSVAL(ret);
		args.rval().set(jsret);

		return true;
	}

	JS_ReportError(cx, "JS_Native_skipBuffer : wrong number of arguments: %d, was expecting %d", argc, 0);
	return false;
}

//自动释放对象
bool JS_Native_create(JSContext *cx, unsigned int argc, jsval *vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (argc == 0)
	{
		auto ret = Wrap::NativeBuffer::Create();
		js_type_class_t *typeClass = js_get_type_from_native<Wrap::NativeBuffer>(ret);
		JS::RootedObject jsret(cx, jsb_ref_autoreleased_create_jsobject(cx, ret, typeClass, "simple::NativeBuffer"));
		args.rval().set(OBJECT_TO_JSVAL(jsret));
		return true;
	}
	JS_ReportError(cx, "JS_Native_create : wrong number of arguments");
	return false;
}

static bool empty_constructor(JSContext *cx, uint32_t argc, jsval *vp)
{
	return false;
}

JSClass *jsb_nativebuffer_class;
JSObject *jsb_nativebuffer_prototype;

void register_NativeBuffer(JSContext *cx, JS::HandleObject ns)
{
	jsb_nativebuffer_class = (JSClass *)calloc(1, sizeof(JSClass));
	jsb_nativebuffer_class->name = "NativeBuffer";
	jsb_nativebuffer_class->addProperty = JS_PropertyStub;
	jsb_nativebuffer_class->delProperty = JS_DeletePropertyStub;
	jsb_nativebuffer_class->getProperty = JS_PropertyStub;
	jsb_nativebuffer_class->setProperty = JS_StrictPropertyStub;
	jsb_nativebuffer_class->enumerate = JS_EnumerateStub;
	jsb_nativebuffer_class->resolve = JS_ResolveStub;
	jsb_nativebuffer_class->convert = JS_ConvertStub;
	jsb_nativebuffer_class->flags = JSCLASS_HAS_RESERVED_SLOTS(2);

	static JSPropertySpec properties[] = {
		JS_PS_END };

	static JSFunctionSpec funcs[] = {
		JS_FN("writeChar", JS_Native_writeChar, 1, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("writeShort", JS_Native_writeShort, 1, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("writeInt", JS_Native_writeInt, 1, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("writeInt64", JS_Native_writeInt64, 1, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("writeFloat", JS_Native_writeFloat, 1, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("writeString", JS_Native_writeString, 1, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("writeStringNoLen", JS_Native_writeStringNoLen, 1, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("writeStringWithUtf8", JS_Native_writeStringWithUtf8, 1, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("getBuffer", JS_Native_getBuffer, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("getBufferLen", JS_Native_getBufferLen, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("clearBuffer", JS_Native_clearBuffer, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("readChar", JS_Native_readChar, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("readShort", JS_Native_readShort, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("readInt", JS_Native_readInt, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("readInt64", JS_Native_readInt64, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("readFloat", JS_Native_readFloat, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("readString", JS_Native_readString, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("readStringNoLen", JS_Native_readStringNoLen, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("readStringWithUtf8", JS_Native_readStringWithUtf8, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("hasData", JS_Native_hasData, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FN("skipBuffer", JS_Native_skipBuffer, 1, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FS_END };

	static JSFunctionSpec st_funcs[] = {
		JS_FN("create", JS_Native_create, 0, JSPROP_PERMANENT | JSPROP_ENUMERATE),
		JS_FS_END };

	jsb_nativebuffer_prototype = JS_InitClass(
		cx, ns,
		JS::NullPtr(),
		jsb_nativebuffer_class,
		empty_constructor, 0, // constructor
		properties,
		funcs,
		NULL, // no static properties
		st_funcs);

	JS::RootedObject proto(cx, jsb_nativebuffer_prototype);
	JS::RootedValue className(cx, std_string_to_jsval(cx, "NativeBuffer"));
	JS_SetProperty(cx, proto, "_className", className);
	JS_SetProperty(cx, proto, "__nativeObj", JS::TrueHandleValue);
	JS_SetProperty(cx, proto, "__is_ref", JS::TrueHandleValue);
	// add the proto and JSClass to the type->js info hash table
	jsb_register_class<Wrap::NativeBuffer>(cx, jsb_nativebuffer_class, proto, JS::NullPtr());
}

#endif // COCOS_PROJECT

