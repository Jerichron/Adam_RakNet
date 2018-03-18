#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <map>
#include <mutex>
#include <string>

static int SERVER_PORT = 65000;
static int CLIENT_PORT = 65001;
static int MAX_CONNECTIONS = 3;
static char ip[64] = "192.168.75.1";

RakNet::SystemAddress g_serverAddress;
RakNet::RakPeerInterface *g_rakPeerInterface = nullptr;

bool isServer = false;
bool isRunning = true;

// packet enums
enum
{
	ID_GAME_LOBBY = ID_USER_PACKET_ENUM,
	ID_PLAYER_READY,
	ID_GAME_START,
	ID_SET_TURN,
	ID_SET_WAITING,
	ID_GAME_OVER,
	ID_REQUEST_ATTACK_INFO, // attack
	ID_REQUEST_PLAYER_INFO,
	ID_ATTACK_INFO, // attack
	ID_ATTACK_INFO_COMPLETE, // attack
	ID_PLAYER_INFO,
	ID_TURN_ATTACK, // attack
	ID_UPDATE_CLIENTS_ATTACK, // attack
	ID_UPDATE_CLIENTS_ATTACK_FAIL, // attack
	ID_TURN_DEFEND, // defend
	ID_UPDATE_CLIENTS_DEFEND // defend
};

enum NetworkStates

{
	NS_Initialize = 0, 
	NS_PendingStart, 
	NS_Started,
	NS_Lobby, 
	NS_Pending, 
	NS_Game 
};

// Server/client instance of the network state, use mutex lock when changing the server's instance
NetworkStates g_networkStates = NS_Initialize;
std::mutex g_networkState_mutex;

// states used by the client
enum TurnState
{
	TS_Waiting = 0, 
	TS_Start,
	TS_Attack,
	TS_Defend,
	TS_Stats
};

TurnState m_turnState = TS_Waiting;
bool isPlayerFinishedTurn = false;
int currentPlayer = 0;

// Player classes not yet implemented
enum PlayerClass
{
	Cleric = 0,
	Mage,
	Rogue,
};

struct SPlayer
{
	std::string m_name;
	int m_damage;
	int m_health;
	PlayerClass m_class;
	bool m_isDefending;
	bool m_Alive;
	int m_index;
	RakNet::SystemAddress m_systemAddress;

	// function to send packet with player info
	void SendPlayerInfo(RakNet::SystemAddress systemAddress, bool isBroadcast)
	{
		RakNet::BitStream bs;
		bs.Write((RakNet::MessageID)ID_PLAYER_READY);
		RakNet::RakString name(m_name.c_str());
		bs.Write(name);
		assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemAddress, isBroadcast));
	}
};

std::map<unsigned long, SPlayer> m_playerMap;
std::map<int, SPlayer> m_playerIndexMap;

// Server side
void OnIncomingConnection(RakNet::Packet* packet)
{
	assert(isServer); // must be the server
	SPlayer tempPlayer = SPlayer();
	tempPlayer.m_systemAddress = packet->systemAddress;
	tempPlayer.m_index = currentPlayer;
	tempPlayer.m_health = 100;
	tempPlayer.m_damage = 25;
	tempPlayer.m_Alive = true;
	tempPlayer.m_isDefending = false;

	// store in map by index
	m_playerIndexMap.insert(std::make_pair(currentPlayer, tempPlayer));
	currentPlayer++;

	// store in map by guid
	m_playerMap.insert(std::make_pair(RakNet::RakNetGUID::ToUint32(packet->guid), tempPlayer));
	std::cout << "Total Players: " << m_playerMap.size() << std::endl;
}

// Client side
void OnConnectionAccepted(RakNet::Packet* packet)
{
	//Ensure is client
	assert(!isServer);
	g_networkState_mutex.lock();
	g_networkStates = NS_Lobby;
	g_networkState_mutex.unlock();
	g_serverAddress = packet->systemAddress;
}

// Server side
SPlayer& GetPlayer(RakNet::RakNetGUID raknetId)
{
	unsigned long guid = RakNet::RakNetGUID::ToUint32(raknetId);
	std::map<unsigned long, SPlayer>::iterator it = m_playerMap.find(guid);
	assert(it != m_playerMap.end());
	return it->second;
}

// Server side
SPlayer& GetPlayer(int playerIndex)
{
	std::map<int, SPlayer>::iterator it = m_playerIndexMap.find(playerIndex);
	assert(it != m_playerIndexMap.end());
	return it->second;
}

// Server side
void OnConnectionLost(RakNet::Packet* packet)
{
	SPlayer& lostPlayer = GetPlayer(packet->guid);
	lostPlayer.SendPlayerInfo(RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	unsigned long keyVal = RakNet::RakNetGUID::ToUint32(packet->guid);
	m_playerMap.erase(keyVal);
}

// Client side
void DisplayPlayerReady(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID msgID;
	bs.Read(msgID);

	RakNet::RakString userName;
	bs.Read(userName);
	std::cout << userName.C_String() << " has joined" << std::endl;
}

// Server side - Send packet for game start (msg, )
void OnGameReady()
{
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)ID_GAME_START);
	std::string temp = "Game is ready to start!";
	RakNet::RakString introText(temp.c_str());
	bs.Write(introText);
	assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true));
}

// Server side
// Called from player on connection
// Displays player's name to all other clients and other client's names to the connecting player
void OnLobbyReady(RakNet::Packet* packet)
{
	unsigned long guid = RakNet::RakNetGUID::ToUint32(packet->guid);
	std::map<unsigned long, SPlayer>::iterator it = m_playerMap.find(guid);
	assert(it != m_playerMap.end());
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID msgID;
	bs.Read(msgID);
	RakNet::RakString userName;
	bs.Read(userName);
	SPlayer& player = it->second;
	player.m_name = userName.C_String();
	
	std::map<int, SPlayer>::iterator it2 = m_playerIndexMap.find(it->second.m_index);
	assert(it2 != m_playerIndexMap.end());
	SPlayer& player2 = it2->second;
	player2.m_name = userName.C_String();

	// Display connecting player
	std::cout << player.m_name.c_str() << " is ready!" << std::endl;
	for (std::map<unsigned long, SPlayer>::iterator it = m_playerMap.begin(); it != m_playerMap.end(); ++it)
	{
		if (guid == it->first)
		{
			continue;
		}
		SPlayer& player = it->second;
		player.SendPlayerInfo(packet->systemAddress, false);
	}
	player.SendPlayerInfo(packet->systemAddress, true);
	// If max connections reached the game is started
	if (g_rakPeerInterface->NumberOfConnections() == MAX_CONNECTIONS)
	{
		g_networkStates = NS_Game;
		OnGameReady();
	}
}

// Client side - display start game info
void DisplayStartGame(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID msgID;
	bs.Read(msgID);
	RakNet::RakString introText;
	bs.Read(introText);

	std::cout << introText.C_String() << std::endl;
	// Update the client's network state
	g_networkStates = NS_Game;
}

// Client side - let the client know its their turn
void OnPlayerTurn(RakNet::Packet* packet)
{
	std::cout << "You're up!" << std::endl;
	m_turnState = TS_Start;
}

void OnPlayerWaiting(RakNet::Packet* packet)
{
	//RakNet::BitStream bs(packet->data, packet->length, false);
	//RakNet::MessageID msgID;
	//bs.Read(msgID);
	//RakNet::RakString name;
	//bs.Read(name);
	//std::cout << name.C_String() << " is up!" << std::endl;

	//-----------------------------------------------------

	char userInput[255];
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID msgID;
	bs.Read(msgID);
	RakNet::RakString name;
	bs.Read(name);
	std::cout << "It is " << name.C_String() << "s turn!" << std::endl;
	std::cout << "input s to see other players info" << std::endl;
	std::cin >> userInput;
	RakNet::BitStream bsAction;

	isServer = userInput[0] == 's';

	switch (userInput[0])
	{
	case 's':
		bsAction.Write((RakNet::MessageID)ID_REQUEST_PLAYER_INFO);
		assert(g_rakPeerInterface->Send(&bsAction, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
		break;
	default:
		break;
	}
}

void OnGameOver()
{
	std::cout << "All opponents defeated...GAME OVER" << std::endl;
}

// display the attack options 
void OnAttackDisplay(RakNet::Packet* packet)
{
	// Get & read the packet
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID msgID;
	bs.Read(msgID);
	int displayIndex;
	bs.Read(displayIndex);
	RakNet::RakString name;
	bs.Read(name);
	int health;
	bs.Read(health);
	std::cout << displayIndex << ". " << name.C_String() << ": " << health << std::endl;
}

// display player info
void OnPlayerDisplay(RakNet::Packet* packet)
{
	// Get & read the packet
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID msgID;
	bs.Read(msgID);
	int displayIndex;
	bs.Read(displayIndex);
	RakNet::RakString name;
	bs.Read(name);
	int health;
	bs.Read(health);
	std::cout << displayIndex << ". " << name.C_String() << ": " << health << std::endl;
}

// enable the client input
void OnAttackDisplayFinish()
{
	m_turnState = TS_Attack;
}

// Client side - receive the attack resolution info from the most recent atack
void OnAttackFinished(RakNet::Packet* packet)
{
	// Get & read the packet
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID msgID;
	bs.Read(msgID);
	RakNet::RakString dName;
	bs.Read(dName);
	RakNet::RakString aName;
	bs.Read(aName);
	int damage;
	bs.Read(damage);
	int health;
	bs.Read(health);
	std::cout << std::endl;
	std::cout << aName.C_String() << " hits " << dName.C_String() << " for " << damage << " damage. " << std::endl;
	std::cout << dName.C_String() << " has " << health << " health remaining" << std::endl << std::endl;
	if (health <= 0)
	{
		std::cout << dName.C_String() << " has died" << std::endl;
	}
}

// Client side - receive the attack resolution info from the most recent atack
void OnAttackFinishedFail(RakNet::Packet* packet)
{
	// Get & read the packet
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID msgID;
	bs.Read(msgID);

	RakNet::RakString dName;
	bs.Read(dName);

	RakNet::RakString aName;
	bs.Read(aName);

	std::cout << std::endl;
	std::cout << dName.C_String() << " resisted " << aName.C_String() << "'s attack" << std::endl << std::endl;
}

// Client side - receive notice that a player has decided to defend
void OnDefenseFinished(RakNet::Packet* packet)
{
	// Get & read the packet
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID msgID;
	bs.Read(msgID);

	RakNet::RakString name;
	bs.Read(name);

	std::cout << std::endl;
	std::cout << name.C_String() << " is defending!" << std::endl << std::endl;
}

void OnPlayerRequestInfo(RakNet::Packet* packet)
{
	int displayIndex = 1;
	// traverse player map (index)
	for (std::map<int, SPlayer>::iterator it = m_playerIndexMap.begin(); it != m_playerIndexMap.end(); ++it)
	{
		SPlayer& player = it->second;

		RakNet::BitStream bs;
		bs.Write((RakNet::MessageID)ID_PLAYER_INFO);
		bs.Write(displayIndex);
		RakNet::RakString name = player.m_name.c_str();
		bs.Write(name);
		bs.Write(player.m_health);

		std::cout << it->second.m_name.c_str() << "'s health is:" << player.m_health << std::endl;
		
		assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true));
		displayIndex++;
	}
}
// Server side - let the server know the client is done their turn - handle their attack
void OnPlayerRequestAttackInfo(RakNet::Packet* packet)
{
	int displayIndex = 1;

	// traverse player map (index)
	for (std::map<int, SPlayer>::iterator it = m_playerIndexMap.begin(); it != m_playerIndexMap.end(); ++it)
	{
		// if not current player
		if (currentPlayer == it->first)
		{
			continue;
		}
		SPlayer& player = it->second;

		RakNet::BitStream bs;
		bs.Write((RakNet::MessageID)ID_ATTACK_INFO);
		bs.Write(displayIndex);
		RakNet::RakString name = player.m_name.c_str();
		bs.Write(name);
		bs.Write(player.m_health);

		std::cout << it->second.m_name.c_str() << "'s info is being sent to the client for attack display" << std::endl;

		assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, GetPlayer(currentPlayer).m_systemAddress, false));
		displayIndex++;
	}

	// enable client input
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)ID_ATTACK_INFO_COMPLETE);
	assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, GetPlayer(currentPlayer).m_systemAddress, false));
}

// send client done turn to server
void OnPlayerAttack(RakNet::Packet* packet)
{
	// Get & read the packet
	RakNet::BitStream bsr(packet->data, packet->length, false);
	RakNet::MessageID msgID;
	bsr.Read(msgID);
	int index;
	bsr.Read(index);

	index--;
	if (index >= currentPlayer)
	{
		index++;
	}

	// Debug
	int temp = currentPlayer + 1;
	int temp2 = index + 1;
	std::cout << "p" << temp2 << " was attacked by p" << temp << std::endl;

	// Get players
	SPlayer& aPlayer = GetPlayer(currentPlayer);
	SPlayer& dPlayer = GetPlayer(index);

	// Check if the player is defending
	if (dPlayer.m_isDefending)
	{
		// Create packet
		RakNet::BitStream bsw;
		bsw.Write((RakNet::MessageID)ID_UPDATE_CLIENTS_ATTACK_FAIL);
		RakNet::RakString dName = dPlayer.m_name.c_str();
		bsw.Write(dName);
		RakNet::RakString aName = aPlayer.m_name.c_str();
		bsw.Write(aName);

		// Broadcast results to all clients
		assert(g_rakPeerInterface->Send(&bsw, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true));
	}

	else
	{
		//update player health
		int damage = aPlayer.m_damage;
		dPlayer.m_health -= damage;
		
		// Create packet
		RakNet::BitStream bsw;
		bsw.Write((RakNet::MessageID)ID_UPDATE_CLIENTS_ATTACK);
		RakNet::RakString dName = dPlayer.m_name.c_str();
		bsw.Write(dName);
		RakNet::RakString aName = aPlayer.m_name.c_str();
		bsw.Write(aName);
		bsw.Write(damage);
		bsw.Write(dPlayer.m_health);
		if (dPlayer.m_health == 0)
		{
			dPlayer.m_Alive = false;
		}
		// Broadcast results to clients
		assert(g_rakPeerInterface->Send(&bsw, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true));
	}
	// Start the next turn
	isPlayerFinishedTurn = true;
}

// Handle defend action
void OnPlayerDefend()
{
	SPlayer& player = GetPlayer(currentPlayer);
	// Set the current player to defending
	player.m_isDefending = true;

	// Create packet
	RakNet::BitStream bsw;
	bsw.Write((RakNet::MessageID)ID_UPDATE_CLIENTS_DEFEND);
	RakNet::RakString name = player.m_name.c_str();
	bsw.Write(name);

	// Broadcast action to all clients
	assert(g_rakPeerInterface->Send(&bsw, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true));
	isPlayerFinishedTurn = true;
}

void InputHandler()
{
	while (isRunning)
	{
		char userInput[255];
		if (g_networkStates == NS_Initialize)
		{
			std::cout << "Press 'S' for Server and 'C' for Client" << std::endl;
			std::cin >> userInput;
			isServer = userInput[0] == 's';

			if (!isServer)
			{
				// Get ip to connect to
				std::cout << "Enter the IP address you would like to connect to" << std::endl;
				std::cin >> ip;
			}
			g_networkState_mutex.lock();
			g_networkStates = NS_PendingStart;
			g_networkState_mutex.unlock();
		}

		else if (g_networkStates == NS_Lobby)
		{
			std::cout << "Enter your name to play or type quit to leave" << std::endl;
			std::cin >> userInput;
			assert(strcmp(userInput, "quit"));// no one leaves the game...NO ONE!

			RakNet::BitStream bs;
			bs.Write((RakNet::MessageID)ID_GAME_LOBBY);
			RakNet::RakString name(userInput);
			bs.Write(name);

			assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
			g_networkState_mutex.lock();
			g_networkStates = NS_Pending;
			g_networkState_mutex.unlock();
		}
		else if (g_networkStates == NS_Pending)
		{
			static bool doOnce = false;
			if (!doOnce)
			{
				std::cout << "Pending game start.." << std::endl;
			}
			doOnce = true;
		}
		else if (g_networkStates == NS_Game)
		{
			if (m_turnState == TS_Start) // server state will allways be TS_Waiting
			{
				m_turnState = TS_Waiting;
				
				// Display turn options
				std::cout << "Choose from the following options (Enter '1', '2', '3', etc.)" << std::endl;
				std::cout << "1. Attack" << std::endl;
				std::cout << "2. Defend" << std::endl << std::endl;
				// Wait for valid input
				bool validInput = false;
				while (!validInput)
				{
					// Get input
					std::cin >> userInput;
					validInput = userInput[0] == '1' || userInput[0] == '2';
					if (!validInput)
						std::cout << "Not valid input - Please enter '1', '2', etc." << std::endl;
				}
				RakNet::BitStream bsAction;
				RakNet::BitStream bsAction2;
				
				switch (userInput[0])
				{
				case '1':
					// Display attack options
					std::cout << "Who would you like to target? (Enter a number or 'b' to go back)" << std::endl;
					// Send action to server
					bsAction.Write((RakNet::MessageID)ID_REQUEST_ATTACK_INFO);
					assert(g_rakPeerInterface->Send(&bsAction, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
					break;
				case '2':
					bsAction2.Write((RakNet::MessageID)ID_TURN_DEFEND);
					assert(g_rakPeerInterface->Send(&bsAction2, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
					break;
				default:
					break;
				}
			}
			else if (m_turnState == TS_Attack)
			{
				m_turnState = TS_Waiting; // end player turn
										 
				bool validInput = false;
				while (!validInput)
				{
					// Get input
					std::cin >> userInput;
					// Check for valid input
					validInput = userInput[0] == '1' || userInput[0] == '2' || userInput[0] == '3' || userInput[0] == '4' || userInput[0] == 'b';
					if (!validInput)
						std::cout << "Not valid input - Please enter '1', '2', '3', etc." << std::endl;
				}
				// Check to go back
				if (userInput[0] == 'b')
				{
					m_turnState = TS_Start;
				}
				else 
				{
					RakNet::BitStream bs;
					bs.Write((RakNet::MessageID)ID_TURN_ATTACK);
					int index = (int)userInput[0] - '0';
					bs.Write(index);
					assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
}

unsigned char GetPacketIdentifier(RakNet::Packet *p)
{
	if (p == 0)
		return 255;
	if ((unsigned char)p->data[0] == ID_TIMESTAMP)
	{
		RakAssert(p->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
		return (unsigned char)p->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
	}
	else
		return (unsigned char)p->data[0];
}

bool HandleLowLevelPacket(RakNet::Packet *packet)
{
	// We got a packet, get the identifier with our handy function
	unsigned char packetIdentifier = GetPacketIdentifier(packet);
	bool isHandled = true;
	switch (packetIdentifier)
	{
	case ID_DISCONNECTION_NOTIFICATION:
		// Connection lost normally
		printf("ID_DISCONNECTION_NOTIFICATION\n");
		break;
	case ID_ALREADY_CONNECTED:
		// Connection lost normally
		printf("ID_ALREADY_CONNECTED with guid %" PRINTF_64_BIT_MODIFIER "u\n", packet->guid);
		break;
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
		break;
	case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server telling the clients of another client disconnecting gracefully.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n");
		break;
	case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_CONNECTION_LOST\n");
		break;
	case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
		OnIncomingConnection(packet);
		break;
	case ID_CONNECTION_BANNED: // Banned from this server
		printf("We are banned from this server.\n");
		break;
	case ID_CONNECTION_ATTEMPT_FAILED:
		printf("Connection attempt failed\n");
		break;
	case ID_NO_FREE_INCOMING_CONNECTIONS:
		// Sorry, the server is full.  I don't do anything here but
		// A real app should tell the user
		printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
		break;
	case ID_INVALID_PASSWORD:
		printf("ID_INVALID_PASSWORD\n");
		break;
	case ID_CONNECTION_LOST:
		// Couldn't deliver a reliable packet - i.e. the other system was abnormally terminated
		printf("ID_CONNECTION_LOST\n");
		OnConnectionLost(packet);
		break;
	case ID_CONNECTION_REQUEST_ACCEPTED:
		// This tells the client they have connected
		printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", packet->systemAddress.ToString(true), packet->guid.ToString());
		printf("My external address is %s\n", g_rakPeerInterface->GetExternalID(packet->systemAddress).ToString(true));
		OnConnectionAccepted(packet);
		break;
	case ID_CONNECTED_PING:
	case ID_UNCONNECTED_PING:
		printf("Ping from %s\n", packet->systemAddress.ToString(true));
		break;
	case ID_NEW_INCOMING_CONNECTION:
		printf("New incoming connection...\n");
		OnIncomingConnection(packet);
		break;
	default:
		isHandled = false;
		break;
	}
	return isHandled;
}

void PacketHandler()
{
	while (isRunning)
	{
		for (RakNet::Packet* packet = g_rakPeerInterface->Receive(); packet != nullptr; g_rakPeerInterface->DeallocatePacket(packet), packet = g_rakPeerInterface->Receive())
		{
			if (!HandleLowLevelPacket(packet))
			{
				// We got a packet, get the identifier with our handy function
				unsigned char packetIdentifier = GetPacketIdentifier(packet);

				// Check if this is a network message packet
				switch (packetIdentifier)
				{
				case ID_GAME_LOBBY:
					OnLobbyReady(packet); 
					break;
				case ID_PLAYER_READY:
					DisplayPlayerReady(packet); 
					break;
				case ID_GAME_START:
					DisplayStartGame(packet); 
					break;
				case ID_SET_TURN: 
					OnPlayerTurn(packet);
					break;
				case ID_SET_WAITING: 
					OnPlayerWaiting(packet);
					break;
				case ID_TURN_ATTACK:
					OnPlayerAttack(packet);
					break;
				case ID_REQUEST_ATTACK_INFO: 
					OnPlayerRequestAttackInfo(packet);
					break;
				case ID_REQUEST_PLAYER_INFO:
					OnPlayerRequestInfo(packet);
					break;
				case ID_ATTACK_INFO:
					OnAttackDisplay(packet);
					break;
				case ID_PLAYER_INFO: 
					OnPlayerDisplay(packet);
					break;
				case ID_ATTACK_INFO_COMPLETE: 
					OnAttackDisplayFinish();
					break;
				case ID_UPDATE_CLIENTS_ATTACK: 
					OnAttackFinished(packet);
					break;
				case ID_UPDATE_CLIENTS_ATTACK_FAIL: 
					OnAttackFinishedFail(packet);
					break;
				case ID_TURN_DEFEND: 
					OnPlayerDefend();
					break;
				case ID_UPDATE_CLIENTS_DEFEND: 
					OnDefenseFinished(packet);
					break;
				case ID_GAME_OVER:
					OnGameOver();
				default:
					break;
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
}

int main()
{
	g_rakPeerInterface = RakNet::RakPeerInterface::GetInstance();

	std::thread inputHandler(InputHandler);
	std::thread packetHandler(PacketHandler);

	while (isRunning)
	{
		if (g_networkStates == NS_PendingStart)
		{
			if (isServer) // server
			{
				// opening up the serverInterface socket
				RakNet::SocketDescriptor socketDescriptors[1];
				socketDescriptors[0].port = SERVER_PORT;
				socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4

				bool isSuccess = g_rakPeerInterface->Startup(MAX_CONNECTIONS, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
				assert(isSuccess);

				g_rakPeerInterface->SetMaximumIncomingConnections(MAX_CONNECTIONS);
				std::cout << "Server Started..." << std::endl;
				g_networkState_mutex.lock();
				g_networkStates = NS_Started;
				g_networkState_mutex.unlock();
			}
			else // client
			{
				// Create socket
				RakNet::SocketDescriptor socketDescriptor(CLIENT_PORT, 0);
				socketDescriptor.socketFamily = AF_INET;

				while (RakNet::IRNS2_Berkley::IsPortInUse(socketDescriptor.port, socketDescriptor.hostAddress, socketDescriptor.socketFamily, SOCK_DGRAM) == true)
				{
					socketDescriptor.port++;
				}
				bool isSuccess = g_rakPeerInterface->Startup(MAX_CONNECTIONS, &socketDescriptor, 1) == RakNet::RAKNET_STARTED;
				assert(isSuccess);

				g_rakPeerInterface->SetOccasionalPing(true);

				RakNet::ConnectionAttemptResult connectionResult = g_rakPeerInterface->Connect(ip, SERVER_PORT, nullptr, 0);
				RakAssert(connectionResult == RakNet::CONNECTION_ATTEMPT_STARTED);

				g_networkStates = NS_Started;
				std::cout << "Client attempted connection.. waiting for response" << std::endl;
			}
		}
		else if (g_networkStates == NS_Game)
		{
			if (isServer)
			{
				// Start the first turn
				static bool doOnce = false;
				if (!doOnce)
				{
					isPlayerFinishedTurn = false;
					// choose a player to begin
					currentPlayer = rand() % MAX_CONNECTIONS;
					
					for (std::map<int, SPlayer>::iterator it = m_playerIndexMap.begin(); it != m_playerIndexMap.end(); ++it)
					{
						if (currentPlayer == it->first) // active player
						{
							RakNet::BitStream bs;
							bs.Write((RakNet::MessageID)ID_SET_TURN);
							// Send packet
							SPlayer& player = GetPlayer(currentPlayer);
							assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, player.m_systemAddress, false));
						}
						else // not active player
						{
							// send the active player's name
							RakNet::BitStream bs;
							bs.Write((RakNet::MessageID)ID_SET_WAITING);
							RakNet::RakString name = GetPlayer(currentPlayer).m_name.c_str();
							bs.Write(name);

							// send packet
							SPlayer& player = it->second;
							assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, player.m_systemAddress, false));
						}
					}
				}
				doOnce = true;

				// Next turn
				if (isPlayerFinishedTurn)
				{
					std::cout << "start next turn" << std::endl;
					isPlayerFinishedTurn = false;
					currentPlayer++;
					
					// check if we've reached the end of the player count
					if (currentPlayer >= MAX_CONNECTIONS)
					{
						currentPlayer = 0;
					}
					
					std::cout << "Current player: " << currentPlayer << std::endl;
					GetPlayer(currentPlayer).m_isDefending = false; // reset defending status

					if (GetPlayer(currentPlayer).m_health <= 0)
					{
						isPlayerFinishedTurn = true;
						std::cout << "Current player is dead moving to the next player" << std::endl;
					}
					// Traverse all players, tell the active player its their turn, and the other player's who is the active player
					int deadPlayers = 0;
					for (std::map<int, SPlayer>::iterator it = m_playerIndexMap.begin(); it != m_playerIndexMap.end(); ++it)
					{

						if (GetPlayer(0).m_health <= 0 && GetPlayer(1).m_health <= 0 || GetPlayer(0).m_health <= 0 && GetPlayer(2).m_health <= 0 ||
							GetPlayer(1).m_health <= 0 && GetPlayer(2).m_health <= 0)
						{
							// send the case for game over
							RakNet::BitStream bs;
							bs.Write((RakNet::MessageID)ID_GAME_OVER);
							SPlayer& player = it->second;
							assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, player.m_systemAddress, false));

						}

						if (currentPlayer == it->first) // active player
						{
							// Just send the msg id, the client knows what to do
							RakNet::BitStream bs;
							bs.Write((RakNet::MessageID)ID_SET_TURN);

							// Send packet
							SPlayer& player = GetPlayer(currentPlayer);
							if (player.m_health <= 0)
							{
								// send the active player's name
								RakNet::BitStream bs;
								bs.Write((RakNet::MessageID)ID_SET_WAITING);
								RakNet::RakString name = GetPlayer(currentPlayer).m_name.c_str();
								bs.Write(name);

								// send packet
								SPlayer& player = it->second;
								assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, player.m_systemAddress, false));
							}
							else
							assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, player.m_systemAddress, false));
						}
						else // not active player
						{
							// send the active player's name
							RakNet::BitStream bs;
							bs.Write((RakNet::MessageID)ID_SET_WAITING);
							RakNet::RakString name = GetPlayer(currentPlayer).m_name.c_str();
							bs.Write(name);

							// send packet
							SPlayer& player = it->second;
							assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, player.m_systemAddress, false));
						}
					}
				}
			}
		}
	}

	inputHandler.join();
	packetHandler.join();

	return 0;
}