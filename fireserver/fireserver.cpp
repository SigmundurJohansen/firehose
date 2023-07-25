#include <iostream>
#include <firehose.h>
#include <fire_server.h>
#include <cstdint>


enum class CustomMsgTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
};
class FireServer : public firehose::ServerInterface< CustomMsgTypes>
{
public:
	FireServer(uint16_t nPort) : firehose::ServerInterface<CustomMsgTypes>(nPort)
	{

	}
protected:
	virtual bool OnClientConnect(std::shared_ptr<firehose::Connection<CustomMsgTypes>> client)
	{
		firehose::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerAccept;
		client->Send(msg);
		return true;
	}
	virtual void OnClientDisconnect(std::shared_ptr<firehose::Connection<CustomMsgTypes>> client)
	{
		std::cout << "Removing client [" << client->GetID() << "]\n";
	}
	// Called when a message arrives
	virtual void OnMessage(std::shared_ptr<firehose::Connection<CustomMsgTypes>> client, firehose::message<CustomMsgTypes>& msg)
	{
		switch (msg.header.id)
		{
		case CustomMsgTypes::ServerPing:
		{
			std::cout << "[" << client->GetID() << "]: Server Ping\n";

			client->Send(msg);
		}
		break;
		case CustomMsgTypes::MessageAll:
		{
			std::cout << "[" << client->GetID() << "]: Message All\n";
			firehose::message<CustomMsgTypes> msg;
			msg.header.id = CustomMsgTypes::ServerMessage;
			msg << client->GetID();
			MessageAllClients(msg, client);
		}
		break;
		}
	}
};

int main()
{
	FireServer myServer(60000);
	myServer.Start();
	while (true)
	{
		myServer.Update(-1, true);
	}
	std::cout << "bye!\n";
	return 0;
}
