#ifndef OIS_DEVICE_INCLUDED
#error "Include oisdevice.h first!"
#endif


class OisWebsocketPort : public IOisPort
{
public:
	bool IsConnected()
	{
		return true;
	}
	void Connect()
	{
	}
	void Disconnect()
	{
	}
	int Read(char* buffer, int size)
	{
		OIS_ASSERT( size >= 0 );
		if( readBuffer.empty() || size <= 0 )
			return 0;
		Frame& f = readBuffer.back();
		OIS_ASSERT( f.cursor <= (int)f.data.size() );
		int remaining = (int)f.data.size() - f.cursor;
		int toCopy = min(size, remaining);
		memcpy(buffer, &f.data.front() + f.cursor, toCopy);
		f.cursor += toCopy;
		if( toCopy == remaining )
		{
			readBuffer.pop_back();
		}
		return toCopy;
	}
	bool Write(const char* buffer, int size)
	{
		OIS_ASSERT( size >= 0 );
		if( size <= 0 )
			return false;
		writeBuffer.push_back({ {}, size });
		writeBuffer.back().data.resize(size);
		memcpy(&writeBuffer.back().data.front(), buffer, size);
		return true;
	}
	
	struct Frame
	{
		OIS_VECTOR<uint8_t> data;
		int cursor;
	};

	OIS_VECTOR<Frame> readBuffer;
	OIS_VECTOR<Frame> writeBuffer;
};