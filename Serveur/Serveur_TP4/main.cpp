#undef UNICODE

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <algorithm>
#include <strstream>
#include <locale>
#include <fstream>
#include <time.h>
#include <sstream>
#include <vector>
#include <queue>
#include <string>
#include <iomanip>

using namespace std;

// link with Ws2_32.lib
#pragma comment( lib, "ws2_32.lib" )





struct userStruct
{
	SOCKET userSd_;
	char* username_;
	char* password_;
	char* adresseIP_;
	int port_;
	char* message_;
};





// External functions
extern DWORD WINAPI EchoHandler(void* user) ;
extern void DoSomething( char *src, char *dest );
extern void RemoveSocket(SOCKET s);
extern char* charDeepCopy(char*, int);

// List of Winsock error constants mapped to an interpretation string.
// Note that this list must remain sorted by the error constants'
// values, because we do a binary search on the list when looking up
// items.
static struct ErrorEntry {
    int nID;
    const char* pcMessage;

    ErrorEntry(int id, const char* pc = 0) : 
    nID(id), 
    pcMessage(pc) 
    { 
    }

    bool operator<(const ErrorEntry& rhs) const
    {
        return nID < rhs.nID;
    }
} gaErrorList[] = {
    ErrorEntry(0,                  "No error"),
    ErrorEntry(WSAEINTR,           "Interrupted system call"),
    ErrorEntry(WSAEBADF,           "Bad file number"),
    ErrorEntry(WSAEACCES,          "Permission denied"),
    ErrorEntry(WSAEFAULT,          "Bad address"),
    ErrorEntry(WSAEINVAL,          "Invalid argument"),
    ErrorEntry(WSAEMFILE,          "Too many open sockets"),
    ErrorEntry(WSAEWOULDBLOCK,     "Operation would block"),
    ErrorEntry(WSAEINPROGRESS,     "Operation now in progress"),
    ErrorEntry(WSAEALREADY,        "Operation already in progress"),
    ErrorEntry(WSAENOTSOCK,        "Socket operation on non-socket"),
    ErrorEntry(WSAEDESTADDRREQ,    "Destination address required"),
    ErrorEntry(WSAEMSGSIZE,        "Message too long"),
    ErrorEntry(WSAEPROTOTYPE,      "Protocol wrong type for socket"),
    ErrorEntry(WSAENOPROTOOPT,     "Bad protocol option"),
    ErrorEntry(WSAEPROTONOSUPPORT, "Protocol not supported"),
    ErrorEntry(WSAESOCKTNOSUPPORT, "Socket type not supported"),
    ErrorEntry(WSAEOPNOTSUPP,      "Operation not supported on socket"),
    ErrorEntry(WSAEPFNOSUPPORT,    "Protocol family not supported"),
    ErrorEntry(WSAEAFNOSUPPORT,    "Address family not supported"),
    ErrorEntry(WSAEADDRINUSE,      "Address already in use"),
    ErrorEntry(WSAEADDRNOTAVAIL,   "Can't assign requested address"),
    ErrorEntry(WSAENETDOWN,        "Network is down"),
    ErrorEntry(WSAENETUNREACH,     "Network is unreachable"),
    ErrorEntry(WSAENETRESET,       "Net connection reset"),
    ErrorEntry(WSAECONNABORTED,    "Software caused connection abort"),
    ErrorEntry(WSAECONNRESET,      "Connection reset by peer"),
    ErrorEntry(WSAENOBUFS,         "No buffer space available"),
    ErrorEntry(WSAEISCONN,         "Socket is already connected"),
    ErrorEntry(WSAENOTCONN,        "Socket is not connected"),
    ErrorEntry(WSAESHUTDOWN,       "Can't send after socket shutdown"),
    ErrorEntry(WSAETOOMANYREFS,    "Too many references, can't splice"),
    ErrorEntry(WSAETIMEDOUT,       "Connection timed out"),
    ErrorEntry(WSAECONNREFUSED,    "Connection refused"),
    ErrorEntry(WSAELOOP,           "Too many levels of symbolic links"),
    ErrorEntry(WSAENAMETOOLONG,    "File name too long"),
    ErrorEntry(WSAEHOSTDOWN,       "Host is down"),
    ErrorEntry(WSAEHOSTUNREACH,    "No route to host"),
    ErrorEntry(WSAENOTEMPTY,       "Directory not empty"),
    ErrorEntry(WSAEPROCLIM,        "Too many processes"),
    ErrorEntry(WSAEUSERS,          "Too many users"),
    ErrorEntry(WSAEDQUOT,          "Disc quota exceeded"),
    ErrorEntry(WSAESTALE,          "Stale NFS file handle"),
    ErrorEntry(WSAEREMOTE,         "Too many levels of remote in path"),
    ErrorEntry(WSASYSNOTREADY,     "Network system is unavailable"),
    ErrorEntry(WSAVERNOTSUPPORTED, "Winsock version out of range"),
    ErrorEntry(WSANOTINITIALISED,  "WSAStartup not yet called"),
    ErrorEntry(WSAEDISCON,         "Graceful shutdown in progress"),
    ErrorEntry(WSAHOST_NOT_FOUND,  "Host not found"),
    ErrorEntry(WSANO_DATA,         "No host data of that type was found")
};
const int kNumMessages = sizeof(gaErrorList) / sizeof(ErrorEntry);
vector<userStruct*> userList;
deque<string> recentMessages;

//// WSAGetLastErrorMessage ////////////////////////////////////////////
// A function similar in spirit to Unix's perror() that tacks a canned 
// interpretation of the value of WSAGetLastError() onto the end of a
// passed string, separated by a ": ".  Generally, you should implement
// smarter error handling than this, but for default cases and simple
// programs, this function is sufficient.
//
// This function returns a pointer to an internal static buffer, so you
// must copy the data from this function before you call it again.  It
// follows that this function is also not thread-safe.
const char* WSAGetLastErrorMessage(const char* pcMessagePrefix, int nErrorID = 0)
{
    // Build basic error string
    static char acErrorBuffer[256];
    ostrstream outs(acErrorBuffer, sizeof(acErrorBuffer));
    outs << pcMessagePrefix << ": ";

    // Tack appropriate canned message onto end of supplied message 
    // prefix. Note that we do a binary search here: gaErrorList must be
	// sorted by the error constant's value.
	ErrorEntry* pEnd = gaErrorList + kNumMessages;
    ErrorEntry Target(nErrorID ? nErrorID : WSAGetLastError());
    ErrorEntry* it = lower_bound(gaErrorList, pEnd, Target);
    if ((it != pEnd) && (it->nID == Target.nID)) {
        outs << it->pcMessage;
    }
    else {
        // Didn't find error in list, so make up a generic one
        outs << "unknown error";
    }
    outs << " (" << Target.nID << ")";

    // Finish error message off and return it.
    outs << ends;
    acErrorBuffer[sizeof(acErrorBuffer) - 1] = '\0';
    return acErrorBuffer;
}

int main(void) 
{
	//----------------------
	// Initialize Winsock.
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR) {
		cerr << "Error at WSAStartup()\n" << endl;
		return 1;
	}

	//----------------------
	// Create a SOCKET for listening for
	// incoming connection requests.
	SOCKET ServerSocket;
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ServerSocket == INVALID_SOCKET) {
        cerr << WSAGetLastErrorMessage("Error at socket()") << endl;
		WSACleanup();
		return 1;
	}
	char option[] = "1";
	setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, option, sizeof(option));

    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port for the socket that is being bound.

	//Récupération du port
	int port;
	do{
		cout << "Entrez le port d'ecoute (entre 5000 et 5050): \n";
		cin >> port;
		if ( cin.fail() || port < 5000 || port > 5050) {
			cout << "Erreur: le port doit etre entre 5000 et 5050, veuillez reessayer. \n";
			cin.clear();
			cin.ignore(10000, '\n');
		}
	}while (port < 5000 || port > 5050);

	hostent *thisHost;

	//Récupération de l'adresse IP
	string ipStr;
	int isValidHost;
	do {
		do {
			printf("Entrez l'adresse IP de l'hote du serveur: \n");
			cin >> ipStr;
			struct sockaddr_in sa;
			isValidHost = inet_pton(AF_INET, ipStr.c_str(), &(sa.sin_addr));
			if(!isValidHost)
				printf("Erreur: adresse IPv4 non conforme.\n");
		} while (!isValidHost);
		printf("Connexion en cours... \n");
		const char* ipChar = ipStr.c_str();
		thisHost = gethostbyname(ipChar);
		if (thisHost == NULL) {
			printf("Erreur: impossible de se connecter a l'adresse IP donnee. \n");
		}
	} while (thisHost == NULL);

	//construction du sockaddr_in
	char* ip;
	ip=inet_ntoa(*(struct in_addr*) *thisHost->h_addr_list);
	printf("Adresse locale trouvee %s : \n\n",ip);
	sockaddr_in service;
    service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr(ip);
    service.sin_port = htons(port);

    if (bind(ServerSocket, (SOCKADDR*) &service, sizeof(service)) == SOCKET_ERROR) {
		cerr << WSAGetLastErrorMessage("bind() failed.") << endl;
		closesocket(ServerSocket);
		WSACleanup();
		return 1;
	}


	//----------------------
	// Listen for incoming connection requests.
	// on the created socket
	if (listen(ServerSocket, 30) == SOCKET_ERROR) {
		cerr << WSAGetLastErrorMessage("Error listening on socket.") << endl;
		closesocket(ServerSocket);
		WSACleanup();
		return 1;
	}


	printf("En attente des connections des clients sur le port %d...\n\n",ntohs(service.sin_port));

    while (true) {	

		sockaddr_in sinRemote;
		int nAddrSize = sizeof(sinRemote);
		// Create a SOCKET for accepting incoming requests.
		// Accept the connection.
		SOCKET sd = accept(ServerSocket, (sockaddr*)&sinRemote, &nAddrSize);

		//Le serveur recoit le nom d'utilisateur et le mot de passe
		char readBuffer;
		char readUsername[80], readPassword[80];
		readBuffer = recv(sd, readUsername, 80, 0); 
		readBuffer = recv(sd, readPassword, 80, 0);

		//Vérification si le socket est valide
        if (sd != INVALID_SOCKET) {
			DWORD nThreadID;

			//Structure contenant les infos de l'utilisateur connecté
			userStruct *user = new userStruct();
			user->userSd_ = sd;
			user->username_ = charDeepCopy(readUsername, strlen(readUsername) + 1);
			user->password_ = charDeepCopy(readPassword, strlen(readPassword) + 1);
			user->adresseIP_ = inet_ntoa(sinRemote.sin_addr);
			user->port_ = port;

			//Création d'un thread pour chaque client connecté
			CreateThread(0, 0, EchoHandler, user, 0, &nThreadID);
        }
        else {
            cerr << WSAGetLastErrorMessage("Echec d'une connection.") << 
                    endl;
           // return 1;
        }
    }
}




//// EchoHandler ///////////////////////////////////////////////////////
// Handles the incoming data by reflecting it back to the sender.

static DWORD WINAPI EchoHandler(void* u)
{
	userStruct* user = static_cast<userStruct*>(u);
	SOCKET sd = (SOCKET)user->userSd_;

	//BD contenant noms d'utilisateur et mots de passe
	fstream fichier;
	fichier.open("infoPersonelle.txt", fstream::in);
	if (!fichier) {
		fichier.open("infoPersonelle.txt", fstream::in | fstream::out | fstream::trunc);
		fichier.close();
		fichier.open("infoPersonelle.txt", fstream::in);
	}

	//Vérification du mot de passe entré par l'utilisateur
	bool foundUser = false;
	bool validCreds = true;
	while (!fichier.eof() && !foundUser) {
		char username[80];
		char password[80];
		fichier >> username;
		fichier >> password;
		cout << user->username_ << ":" << username << "\n";
		if (strcmp(user->username_, username) == 0) {
			foundUser = true;
			if (strcmp(user->password_, password) != 0) {
				send(sd, "0", 1, 0);
				validCreds = false;
				cout << "Connection refusee De : " <<
					user->adresseIP_ << ":" <<
					user->port_ << "." <<
					endl;
				RemoveSocket(sd);
				return 0;
			}
		}
	}
	fichier.close();

	//Création d'un nouvel utilisateur si celui-ci n'a pas été trouvé dans le fichier
	fichier.open("infoPersonelle.txt", fstream::out | fstream::app);
	if (!foundUser) {
		cout << "write" << "\n";
		fichier.seekp(0, ios_base::end);
		fichier << user->username_ << " " << user->password_ << "\n";
	}
	fichier.close();
	fichier.open("infoPersonelle.txt", fstream::in);

	//Si tout se passe bien, on accepte la connection et on ajoute l'usager à la liste de client connectés
	if (validCreds) {
		send(sd, "1", 1, 0);
		cout << "Connection acceptee De : " <<
			user->adresseIP_ << ":" <<
			user->port_ << "." <<
			endl;

		userList.push_back(user);
	}

	//Réservation d'espaces mémoire pour la réception et l'envoi de données
	char readBuffer[200], outBuffer[330];
	memset(readBuffer, 0, sizeof(readBuffer));
	memset(outBuffer, 0, sizeof(outBuffer));

	//Envoie les 15 derniers messages
	for (deque<string>::iterator it = recentMessages.begin(); it != recentMessages.end(); ++it) {
		strcpy(outBuffer, it->c_str());
		send(user->userSd_, outBuffer, sizeof(outBuffer)/ sizeof(outBuffer[0]), 0);
		memset(outBuffer, 0, sizeof(outBuffer));
	}


	int readBytes;
	while ( strcmp(readBuffer,"___DISCONNECT___") != 0) {
		// Read Data from client
		memset(readBuffer, 0, sizeof(readBuffer));
		readBytes = recv(sd, readBuffer, 200, 0);
		user->message_ = readBuffer;
		if (readBytes > 0 && strcmp(readBuffer, "___DISCONNECT___") != 0) {

			stringstream replyBuf;
			time_t t = time(0);   // get time now
			struct tm * now = localtime(&t);

			replyBuf << "[" << user->username_ << " - " << user->adresseIP_ << ":" << user->port_ << " - " <<
				        (now->tm_year + 1900) << "-" << setfill('0') << setw(2) << (now->tm_mon + 1) << "-" <<
						setfill('0') << setw(2) << now->tm_mday << "@" << setfill('0') << setw(2) << now->tm_hour <<
						":" << setfill('0') << setw(2) << now->tm_min << ":" << setfill('0') << setw(2) << now->tm_sec <<
						"]: " << user->message_;
			strcpy(outBuffer,replyBuf.str().c_str());
			for(vector<userStruct*>::iterator it = userList.begin(); it != userList.end(); ++it){
				send((*it)->userSd_, outBuffer, sizeof(outBuffer) / sizeof(outBuffer[0]), 0);
			}
			//enqueue message
			string sMessage = outBuffer;
			//record last 15 messages
			recentMessages.push_back(sMessage);
			if (recentMessages.size() > 15) {
				recentMessages.pop_front();
			}
			//Record messages in history
			fstream fichier;
			fichier.open("MessageHistory.txt", fstream::out | fstream::app);
			if (!fichier) {
				fichier.open("MessageHistory.txt", fstream::out | fstream::out | fstream::trunc);
			}
			fichier << sMessage << endl;
			fichier.close();
		}
		else if (readBytes == SOCKET_ERROR) {
			cout << WSAGetLastErrorMessage("Echec de la reception !") << endl;
			RemoveSocket(sd);
			return 0;
		}
			
	}
	RemoveSocket(sd);
	return 0;
	terminate();
}

//Déconnecte un utilisateur du serveur(côté serveur seulement!).
void RemoveSocket(SOCKET s){
	bool foundSocket = false;
	for (vector<userStruct*>::iterator it = userList.begin(); it != userList.end() && !foundSocket; ++it) {
		if ((*it)->userSd_ == s) {
			cout << "User " << (*it)->username_ << " disconnected from server.\n";
			userList.erase(it);
			foundSocket = true;
		}
	}
	closesocket(s);
}

//Deep copy pour les c-string
char* charDeepCopy(char* source, int size) {
	char* arr2 = new char[size + 1];
	for (int i = 0; i < size; ++i) {
		arr2[i] = source[i];
	}
	return arr2;
}
