#ifndef Buffer_HG
#define Buffer_HG
#include<vector>
class Buffer {
public:
	Buffer(size_t size);
	Buffer();
	~Buffer();
	//int
	void WriteInt32BE(size_t index, int32_t value);
	void WriteInt32BE(int32_t value);
	int ReadInt32BE(size_t index);
	int ReadInt32BE(void);

	//unsigned short
	void WriteUShortBE(size_t index, unsigned short value);
	void WriteUShortBE(unsigned short value);
	unsigned short ReadUShortBE(size_t index);
	unsigned short ReadUShortBE(void);

	//short
	void WriteShortBE(size_t index, short value);
	void WriteShortBE(short value);
	short ReadShortBE(size_t index);
	short ReadShortBE(void);

	//TO DO: string conversion(not really converting anything)
	//void WriteStringBE(size_t index, std::string value);
	void WriteStringBE(std::string value);
	std::string ReadStringBE(size_t index, int length);
	std::string ReadStringBE(int length);
	int GetBufferLength();
	//std::vector<uint8_t> getBuffer();
	std::vector<char>& getBuffer();
	char* Buffer::getBufferAsCharArray();
private:
	std::vector<char> mBuffer;
	int mReadIndex;
	int mWriteIndex;
};
#endif // !Buffer_HG
