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

class OisWebsocketConnection
{
public:
	OisWebsocketConnection(const OIS_STRING& name, unsigned gameVersion, const char* gameName)
		: m_device(m_port, name, gameVersion, gameName)
	{
	}
	OisWebsocketPort m_port;
	OisDevice m_device;
};

#include "webby/webby.h"

class OisWebsocketHost
{
public:
	OisWebsocketHost(unsigned gameVersion, const char* gameName, unsigned short port = 8080)
		: m_gameName(gameName)
		, m_gameVersion(gameVersion)
	{
		WORD wsa_version = MAKEWORD(2, 2);
		WSADATA wsa_data;
		if( 0 != WSAStartup( wsa_version, &wsa_data ) )
			return;
		
		WebbyServerConfig config = {};
		config.bind_address = "127.0.0.1";
		config.listening_port = port;
		config.flags = WEBBY_SERVER_WEBSOCKETS;
		config.connection_max = 4;
		config.request_buffer_size = 2048;
		config.io_buffer_size = 8192;
		config.user_host_data = this;
		config.dispatch = &webby_dispatch;
		config.ws_connect = &webby_ws_connect;
		config.ws_connected = &webby_ws_connected;
		config.ws_closed = &webby_ws_closed;
		config.ws_frame = &webby_ws_frame;
		config.ws_poll = &webby_ws_poll;
#ifdef _DEBUG
		config.flags |= WEBBY_SERVER_LOG_DEBUG;
		config.log = &webby_log;
#endif

		int size = WebbyServerMemoryNeeded( &config );
		m_memory.resize(size);
		m_webby = WebbyServerInit( &config, &m_memory.front(), size );
	}
	~OisWebsocketHost()
	{
		WSACleanup();
	}

	void Poll()
	{
		if( m_webby )
			WebbyServerUpdate( m_webby );
	}
	
	const OIS_VECTOR<OisWebsocketConnection*>& Connections() const { return m_connections; }
private:
	OIS_VECTOR<OisWebsocketConnection*> m_connections;
	OIS_VECTOR<char> m_memory;
	WebbyServer* m_webby = nullptr;
	const char* m_gameName;
	unsigned m_gameVersion;
	
	static void webby_log(const char* text)
	{
		OIS_INFO( "Webby: %s", text);
	}

	static int webby_dispatch(struct WebbyConnection *connection)
	{
		OIS_INFO( "[webby_dispatch] url:%s", connection->request.uri);
		bool handled = false;
		return handled?0:1;
	}
	static int webby_ws_connect(struct WebbyConnection *connection)
	{
		OIS_INFO( "[webby_ws_connect] method:%s", connection->request.method);
		OIS_INFO( "[webby_ws_connect] uri:%s", connection->request.uri);
		OIS_INFO( "[webby_ws_connect] http_version:%s", connection->request.http_version);
		OIS_INFO( "[webby_ws_connect] query_params:%s", connection->request.query_params);
		OIS_INFO( "[webby_ws_connect] content_length:%d", connection->request.content_length);
		for( int i=0; i!=connection->request.header_count; ++i )
		{
			OIS_INFO( "[webby_ws_connect] header[%d]: %s = %s", i, connection->request.headers[i].name, connection->request.headers[i].value);
		}

		OisWebsocketHost& self = *(OisWebsocketHost*)connection->user_host_data;
		
		bool handled = false;
		if( 0==strcmp(connection->request.uri, "/input") )
		{
			OisWebsocketConnection* oisConnection = new OisWebsocketConnection("WebSocket", self.m_gameVersion, self.m_gameName);
			connection->user_data = oisConnection;
			self.m_connections.push_back( oisConnection );
			handled = true;
		}
		return handled?0:1;
	}
	static void webby_ws_connected(struct WebbyConnection *connection)
	{
		OIS_INFO( "[webby_ws_connected] url:%s", connection->request.uri);
	}
	static void webby_ws_closed(struct WebbyConnection *connection)
	{
		OIS_INFO( "[webby_ws_closed] url:%s", connection->request.uri);
		OisWebsocketHost& self = *(OisWebsocketHost*)connection->user_host_data;
		OisWebsocketConnection* oisConnection = (OisWebsocketConnection*)connection->user_data;
		self.m_connections.erase( std::find(self.m_connections.begin(), self.m_connections.end(), oisConnection) );
		connection->user_data = 0;
		delete oisConnection;
	}
	static int webby_ws_frame(struct WebbyConnection *connection, const struct WebbyWsFrame *frame)
	{
		OIS_INFO( "[webby_ws_frame] url:%s", connection->request.uri);
		OisWebsocketConnection* oisConnection = (OisWebsocketConnection*)connection->user_data;
		int size = frame->payload_length;
		OIS_VECTOR<uint8_t> buffer;
		buffer.resize(size);
		int r = WebbyRead( connection, &buffer.front(), size );
		if( r == 0 )
		{
			OIS_VECTOR<OisWebsocketPort::Frame>& portBuffer = oisConnection->m_port.readBuffer;
			portBuffer.push_back({});
			std::swap(portBuffer.back().data, buffer);
		}
		return r;
	}
	static void webby_ws_poll(struct WebbyConnection *connection)
	{
		OisWebsocketConnection* oisConnection = (OisWebsocketConnection*)connection->user_data;
		OIS_VECTOR<OisWebsocketPort::Frame>& portBuffer = oisConnection->m_port.writeBuffer;
		for( OisWebsocketPort::Frame& f : portBuffer )
		{
			WebbyBeginSocketFrame( connection, WEBBY_WS_OP_BINARY_FRAME );
			WebbyWrite( connection, &f.data.front(), f.data.size() );
			WebbyEndSocketFrame( connection );
		}
		portBuffer.clear();
	}

};