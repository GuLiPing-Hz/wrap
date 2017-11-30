#ifndef JSON_VALUE_H___
#define JSON_VALUE_H___

#include <vector>
#include <string>

namespace Wrap{

	class BufferValue{
	public:
		enum eDataType {
			type_unknow = 0,
			type_char = 1,//[16+1][数组长度][data]
			type_short = 2,
			type_int = 3,
			type_int64 = 4,
			type_float = 5,//[]
			//17需要单独拎出来
			type_str = 17,

			//下面两个主要给BufferUnitArra使用
			type_custom = 6,//[16+6][数组长度][{结构长度,结构体}]...
			type_array = 16,//
		};
		eDataType type;

		struct Data{
			union BaseData {//联合数据结构
				char c;
				short s;
				int i;
				long long ll;
				float f;
			};
			BaseData base;
			//字符串需要另外存放
			std::string str;
		};
		Data data;

		typedef std::vector<BufferValue*> VECTORBJ;
		VECTORBJ list;

		//BufferJson() :PoolObj("BufferJson"){}

		// 	virtual void reuse(){
		// 		PoolObj::reuse();
		// 		clear();
		// 	}

		void clear(){
			type = type_unknow;
			list.clear();
		}
	};

	void ReleaseBufferValue(BufferValue* &value);

}
#endif // !JSON_VALUE_H___

