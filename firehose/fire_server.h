#pragma once
#include "firehose.h"
#include "fire_standard.h"
#include "fire_tsqueue.h"
#include "fire_message.h"
#include "fire_connection.h"
#include <memory>

namespace firehose
{
	template<typename T>
	class ServerInterface
	{
	public:
		ServerInterface(uint16_t port) : m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
		{

		}
		virtual ~ServerInterface() { Stop(); }

		bool Start()
		{
			try
			{
				WaitForClientConnection();
				m_thread = std::thread([this]() {m_asioContext.run(); });
			}
			catch (std::exception& error)
			{
				std::cerr << "[SERVER] Exception: " << error.what() << "\n";
				return false;
			}

			std::cout << "[SERVER] Started!\n";
			return true;
		}

		void Stop()
		{
			m_asioContext.stop();
			if (m_thread.joinable())
				m_thread.join();
			std::cout << "[SERVER] Stopped!\n";
		}
		void WaitForClientConnection()
		{
			m_asioAcceptor.async_accept([this](std::error_code error, asio::ip::tcp::socket socket)
				{
					if (!error)
					{
						std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";
						
						std::shared_ptr<Connection<T>> newConn = 
							std::make_shared<Connection<T>> (Connection<T>::owner::server, m_asioContext, std::move(socket), m_queueMessageIn);

						if (OnClientConnect(newConn))
						{
							m_dequeConnections.push_back(std::move(newConn));
							m_dequeConnections.back()->ConnectToClient(nIDCounter++);
							std::cout << "[" << m_dequeConnections.back()->GetID() << "] Connection Approved\n";
						}
						else
						{
							std::cout << "[------] COnnection Denied!\n";
						}
					}
					else
					{
						std::cout << "[SERVER] New Connection Error: " << error.message() << "\n";
					}
					WaitForClientConnection();
				}
			);
		}

		void MessageClient(std::shared_ptr<Connection<T>> client, const message<T>& msg)
		{
			if (client && client->IsConnected())
			{
				client->Send(msg);
			}
			else
			{
				OnClientDisconnect(client);
				client.reset();
				m_dequeConnections.erase(std::remove(m_dequeConnections.begin(), m_dequeConnections.end(), client), m_dequeConnections.end());
			}
		}
		void MessageAllClients(const message<T>& msg, std::shared_ptr<Connection<T>> pIgnoreClient = nullptr)
		{
			bool bInvalidClientExists = false;
			for (auto& client : m_dequeConnections)
			{
				if (client && client->IsConnected())
				{
					if (client != pIgnoreClient)
						client->Send(msg);
				}
				else
				{
					OnClientDisconnect(client);
					client.reset();
					bInvalidClientExists = true;
				}
			}
			if (bInvalidClientExists)
				m_dequeConnections.erase(std::remove(m_dequeConnections.begin(), m_dequeConnections.end(), nullptr), m_dequeConnections.end());
		}
		void Update(size_t nMaxMessages = -1, bool bWait = false)
		{
			if (bWait) m_queueMessageIn.wait();
			size_t nMessageCount = 0;
			while (nMessageCount < nMaxMessages && !m_queueMessageIn.empty())
			{
				auto msg = m_queueMessageIn.pop_front();
				OnMessage(msg.remote, msg.msg);
				nMessageCount++;
			}
		}
	protected:
		virtual bool OnClientConnect(std::shared_ptr<Connection<T>> client)
		{
			return false;
		}
		virtual void OnClientDisconnect(std::shared_ptr<Connection<T>> client)
		{

		}
		virtual void OnMessage(std::shared_ptr<Connection<T>> client, message<T>& msg)
		{

		}

		TSQueue<owned_message<T>> m_queueMessageIn;
		std::deque<std::shared_ptr<Connection<T>>> m_dequeConnections;
		asio::io_context m_asioContext;
		std::thread m_thread;
		asio::ip::tcp::acceptor m_asioAcceptor;
		uint32_t nIDCounter = 10000;

	};
}