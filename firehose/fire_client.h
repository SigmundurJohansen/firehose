#pragma once
#include "firehose.h"
#include "fire_connection.h"
#include "fire_standard.h"
#include "fire_message.h"

namespace firehose
{
	template<typename T>
	class ClientInterface
	{
	public:
		ClientInterface()
		{}

		virtual ~ClientInterface()
		{
			Disconnect();
		}

		bool Connect(const std::string& host, const uint16_t port)
		{
			try
			{
				asio::ip::tcp::resolver resolver(m_context);
				asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));
				m_connection = std::make_unique<Connection<T>>(Connection<T>::owner::client, m_context, asio::ip::tcp::socket(m_context), m_messageIn);
				m_connection->ConnectToServer(endpoints);
				m_thread = std::thread([this]() { m_context.run(); });
			}
			catch (std::exception& e)
			{
				std::cerr << "Client exception: " << e.what() << "\n";
				return false;
			}
			return true;
		}

		void Disconnect()
		{
			if (IsConnected())
			{
				m_connection->Disconnect();
			}
			m_context.stop();
			if (m_thread.joinable())
				m_thread.join();

			m_connection.release();
		}

		void Send(const message<T>& msg)
		{
			if (IsConnected())
				m_connection->Send(msg);
		}

		bool IsConnected()
		{
			if (m_connection)
				return m_connection->IsConnected();
			else
				return false;
		}

		TSQueue<owned_message<T>>& Incoming()
		{
			return m_messageIn;
		}

	protected:
		asio::io_context m_context;
		std::thread m_thread;
		std::unique_ptr<Connection<T>> m_connection;
	private:
		TSQueue<owned_message<T>> m_messageIn;
	};
}