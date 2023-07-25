#pragma once
#include "firehose.h"
#include "fire_message.h"
#include "fire_tsqueue.h"

namespace firehose
{
	template<typename T>
	class Connection : public std::enable_shared_from_this<Connection<T>>
	{
	public:
		enum class owner
		{
			server,
			client
		};
		Connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, TSQueue<owned_message<T>>& qIn)
			: m_asioContext(asioContext), m_socket(std::move(socket)), m_queueMessageIn(qIn)
		{
			m_nOwnerType = parent;
		}
		uint32_t GetID() const
		{
			return id;
		}

		virtual ~Connection() {}

		void ConnectToClient(uint32_t uid = 0)
		{
			if (m_nOwnerType == owner::server)
			{
				if (m_socket.is_open())
				{
					id = uid;
					ReadHeader();
				}
			}
		}

		void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
		{
			// Only clients can connect to servers
			if (m_nOwnerType == owner::client)
			{
				// Request asio attempts to connect to an endpoint
				asio::async_connect(m_socket, endpoints,
					[this](std::error_code error, asio::ip::tcp::endpoint endpoint)
					{
						if (!error)
						{
							ReadHeader();
						}
					});
			}
		}
		void Disconnect()
		{
			if (IsConnected())
				asio::post(m_asioContext, [this]() { m_socket.close(); });
		}
		bool IsConnected() const
		{
			return m_socket.is_open();
		}

		void StartListening()
		{

		}

		void Send(const message<T>& msg)
		{
			asio::post(m_asioContext, [this, msg]()
				{
					// If the queue has a message in it, then we must 
					// assume that it is in the process of asynchronously being written.
					// Either way add the message to the queue to be output. If no messages
					// were available to be written, then start the process of writing the
					// message at the front of the queue.
					bool bWritingMessage = !m_queueMessageOut.empty();
					m_queueMessageOut.push_back(msg);
					if (!bWritingMessage)
					{
						WriteHeader();
					}
				});
		}
	private:

		void ReadHeader()
		{
			asio::async_read(m_socket, asio::buffer(&m_msgTempIn.header, sizeof(message_header<T>)),
				[this](std::error_code error, std::size_t length)
				{
					if (!error)
					{
						if (m_msgTempIn.header.size > 0)
						{
							m_msgTempIn.body.resize(m_msgTempIn.header.size);
							ReadBody();
						}
						else
						{
							AddToIncomingMessageQueue();
						}
					}
					else
					{
						std::cout << "[" << id << "] Read Header Failed.\n";
						m_socket.close();
					}
				});
		}
		void ReadBody()
		{
			asio::async_read(m_socket, asio::buffer(m_msgTempIn.body.data(), m_msgTempIn.body.size()),
				[this](std::error_code error, std::size_t length)
				{
					if (!error)
					{
						AddToIncomingMessageQueue();
					}
					else
					{
						std::cout << "[" << id << "[ Ready Body Fail.\n";
						m_socket.close();
					}
				});
		}

		void WriteHeader()
		{
			asio::async_write(m_socket, asio::buffer(&m_queueMessageOut.front().header, sizeof(message_header<T>)),
				[this](std::error_code error, std::size_t length)
				{
					if (!error)
					{
						if (m_queueMessageOut.front().body.size() > 0)
						{
							WriteBody();
						}
						else
						{
							m_queueMessageOut.pop_front();
							if (!m_queueMessageOut.empty())
							{
								WriteHeader();
							}
						}
					}
					else
					{
						std::cout << "[" << id << "] Write Header Fail.\n";
						m_socket.close();
					}
				});
		}

		void WriteBody()
		{
			asio::async_write(m_socket, asio::buffer(m_queueMessageOut.front().body.data(), m_queueMessageOut.front().body.size()),
				[this](std::error_code error, std::size_t length)
				{
					if (!error)
					{
						m_queueMessageOut.pop_front();
						if (!m_queueMessageOut.empty())
						{
							WriteHeader();
						}
					}
					else
					{
						std::cout << "[" << id << "] Write Body Fail. \n";
						m_socket.close();
					}
				});
		}

		void AddToIncomingMessageQueue()
		{
			if (m_nOwnerType == owner::server)
				m_queueMessageIn.push_back({ this->shared_from_this(), m_msgTempIn });
			else
				m_queueMessageIn.push_back({ nullptr, m_msgTempIn });

			ReadHeader();
		}
	protected:
		owner m_nOwnerType = owner::server;
		uint32_t id = 0;
		asio::ip::tcp::socket m_socket;
		asio::io_context& m_asioContext;
		TSQueue<message<T>> m_queueMessageOut;
		TSQueue<owned_message<T>>& m_queueMessageIn;
		message<T> m_msgTempIn;
	};

}