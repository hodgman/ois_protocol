#ifndef OIS_DEVICE_INCLUDED
#error "Include oisdevice.h first!"
#endif

#ifndef OIS_VIRTUAL_PORT
#error "OisWebsocketPort uses the IOisPort interface. Define OIS_VIRTUAL_PORT to opt in"
#endif

#ifndef OIS_WEBBY_INFO
#define OIS_WEBBY_INFO(...) 
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
	virtual const char* Name()
	{
		return "Websocket";
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
	std::vector<const char*> m_eventLog;
	bool abort = false;
};

#include "webby/webby.h"

struct OisWebWhitelist
{
	const char* request;
	const char* path;
	bool isPattern;
};

class OisWebHost
{
public:
	template<unsigned N>
	OisWebHost(unsigned gameVersion, const char* gameName, const OisWebWhitelist(&files)[N], bool allowIndex, unsigned short port = 8080)
		: m_gameName(gameName)
		, m_gameVersion(gameVersion)
		, m_files(files)
		, m_numFiles(N)
		, m_allowIndex(allowIndex)
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
	~OisWebHost()
	{
		WSACleanup();
	}

	void Poll()
	{
		if( m_webby )
			WebbyServerUpdate( m_webby );
	}
	
	const OIS_VECTOR<OisWebsocketConnection*>& Connections() const { return m_connections; }
	bool Disconnect(const OisDevice& d)
	{
		auto it = std::find_if(m_connections.begin(), m_connections.end(), [&d](OisWebsocketConnection* item)
		{
			return &item->m_device == &d;
		});
		if( it == m_connections.end() )
			return false;
		
		(*it)->abort = true;
		return true;
	}
private:
	OIS_VECTOR<OisWebsocketConnection*> m_connections;
	OIS_VECTOR<char> m_memory;
	WebbyServer* m_webby = nullptr;
	const OisWebWhitelist* m_files = nullptr;
	unsigned m_numFiles = 0;
	const char* m_gameName;
	unsigned m_gameVersion;
	bool m_allowIndex;
	
	static void webby_log(const char* text)
	{
		OIS_WEBBY_INFO( "Webby: %s", text);
	}

	static int webby_dispatch(struct WebbyConnection* connection)
	{
		OisWebHost& self = *(OisWebHost*)connection->user_host_data;
		if (!self.m_numFiles)
			return 1;

		const char* uri = connection->request.uri;
		OIS_WEBBY_INFO( "[webby_dispatch] url:%s", uri);

		OIS_STRING_BUILDER sb;
		const char* whitelistedPath = 0;
		for (unsigned i = 0, end = self.m_numFiles; i != end && !whitelistedPath; ++i)
		{
			const OisWebWhitelist& f = self.m_files[i];
			if (f.isPattern)
			{
				if (0 == strncmp(uri, f.request, strlen(f.request)))
					whitelistedPath = sb.FormatTemp("%s%s", f.path, uri);
			}
			else if (0 == strcmp(uri, f.request))
					whitelistedPath = f.path;
		}

		if (!whitelistedPath && self.m_allowIndex && 0 == strcmp(uri, "/"))
		{
			if (WebbyBeginResponse(connection, 200, -1, 0, 0))
				return -1;
			const char* htmlHead =
R"(<!doctype html>
<html>
<head>
	<meta charset="UTF-8">
	<title>Open Interactivity System Hub</title>
</head>
<body>
<div id="container">
	<h1>Available Controllers</h1>
	<ul>
)";
			const char* htmlFoot =
R"(	</ul>
</div>
</body>
</html>)";
			WebbyWrite(connection, htmlHead, strlen(htmlHead));
			for (unsigned i = 0, end = self.m_numFiles; i != end; ++i)
			{
				const OisWebWhitelist& f = self.m_files[i];
				if (f.isPattern)
					continue;
				size_t len = strlen(f.path);
				if (len > 5 && 0 == strcmp(f.path + len - 5, ".html"))
				{
					const char* name = f.path;
					const char* htmlLine = sb.FormatTemp(
R"(	<li><a href="%s">%s</a></li>
)", f.request, name);
					WebbyWrite(connection, htmlLine, strlen(htmlLine));
				}
			}
			WebbyWrite(connection, htmlFoot, strlen(htmlFoot));
			WebbyEndResponse(connection);
		}
		else
		{
			FILE* fp;
			if (whitelistedPath && (fp = fopen(whitelistedPath, "rb")))
			{
				fseek(fp, 0L, SEEK_END);
				long size = ftell(fp);
				rewind(fp);
				if (WebbyBeginResponse(connection, 200, size, 0, 0))
				{
					fclose(fp);
					return -1;
				}
				std::vector<char> data(size);
				char* buffer = &data.front();
				fread(buffer, 1, size, fp);
				WebbyWrite(connection, buffer, size);
				WebbyEndResponse(connection);
				fclose(fp);
			}
			else
			{
				static const char failMessage[] = "404";
				if (WebbyBeginResponse(connection, 404, sizeof(failMessage), 0, 0))
					return -1;
				WebbyWrite(connection, failMessage, sizeof(failMessage));
				WebbyEndResponse(connection);
			}
		}
		return 0;
	}
	static int webby_ws_connect(struct WebbyConnection* connection)
	{
		OIS_WEBBY_INFO( "[webby_ws_connect] method:%s", connection->request.method);
		OIS_WEBBY_INFO( "[webby_ws_connect] uri:%s", connection->request.uri);
		OIS_WEBBY_INFO( "[webby_ws_connect] http_version:%s", connection->request.http_version);
		OIS_WEBBY_INFO( "[webby_ws_connect] query_params:%s", connection->request.query_params);
		OIS_WEBBY_INFO( "[webby_ws_connect] content_length:%d", connection->request.content_length);
		for( int i=0; i!=connection->request.header_count; ++i )
		{
			OIS_WEBBY_INFO( "[webby_ws_connect] header[%d]: %s = %s", i, connection->request.headers[i].name, connection->request.headers[i].value);
		}

		OisWebHost& self = *(OisWebHost*)connection->user_host_data;
		
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
	static void webby_ws_connected(struct WebbyConnection* connection)
	{
		OIS_WEBBY_INFO( "[webby_ws_connected] url:%s", connection->request.uri);
	}
	static void webby_ws_closed(struct WebbyConnection *connection)
	{
		OIS_WEBBY_INFO( "[webby_ws_closed] url:%s", connection->request.uri);
		OisWebHost& self = *(OisWebHost*)connection->user_host_data;
		OisWebsocketConnection* oisConnection = (OisWebsocketConnection*)connection->user_data;
		self.m_connections.erase( std::find(self.m_connections.begin(), self.m_connections.end(), oisConnection) );
		connection->user_data = 0;
		delete oisConnection;
	}
	static int webby_ws_frame(struct WebbyConnection* connection, const struct WebbyWsFrame* frame)
	{
		OIS_WEBBY_INFO( "[webby_ws_frame] url:%s", connection->request.uri);
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
	static int webby_ws_poll(struct WebbyConnection* connection)
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

		return oisConnection->abort ? 1 : 0;
	}

};