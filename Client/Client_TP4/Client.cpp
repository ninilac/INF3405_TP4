#undef UNICODE

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include<stdio.h>

// Link avec ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

#ifdef WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

extern DWORD WINAPI recvMessage(void*);
extern int intDigit(int);

void SetStdinEcho(bool enable = true)
{
#ifdef WIN32
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode;
	GetConsoleMode(hStdin, &mode);

	if (!enable)
		mode &= ~ENABLE_ECHO_INPUT;
	else
		mode |= ENABLE_ECHO_INPUT;

	SetConsoleMode(hStdin, mode);

#else
	struct termios tty;
	tcgetattr(STDIN_FILENO, &tty);
	if (!enable)
		tty.c_lflag &= ~ECHO;
	else
		tty.c_lflag |= ECHO;

	(void)tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#endif
}

int __cdecl main(int argc, char **argv)
{
    WSADATA wsaData;
    SOCKET leSocket;// = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    char motEnvoye[200];
	char motRecu[330];
    int iResult;

	//--------------------------------------------
    // InitialisATION de Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("Erreur de WSAStartup: %d\n", iResult);
        return 1;
    }
	// On va creer le socket pour communiquer avec le serveur
	leSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (leSocket == INVALID_SOCKET) {
        printf("Erreur de socket(): %ld\n\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();
        return 1;
	}
	//--------------------------------------------
	// On va chercher l'adresse du serveur en utilisant la fonction getaddrinfo.
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_INET;        // Famille d'adresses
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;  // Protocole utilisé par le serveur

	// On indique le nom et le port du serveur auquel on veut se connecter
	//char *host = "L4708-XX";
	//char *host = "L4708-XX.lerb.polymtl.ca";
	//char *host = "add_IP locale";

	//Récupération et vérification de l'adresse IP donnée par l'utilisateur
	char host[16];
	int isValidHost;
	do {
		printf("Saisir l'adresse IP du serveur auquel vous voulez vous connecter: \n");
		gets_s(host);
		struct sockaddr_in sa;
		isValidHost = inet_pton(AF_INET, host, &(sa.sin_addr));
		if (!isValidHost) {
			printf("Erreur: adresse IPv4 non conforme.\n");
		}
	} while (!isValidHost);

	//Récupération et vérification du port donné par l'utilisateur
	char port[5];
	int portInt;
	bool validPort;
	do {
		validPort = true;
		printf("Entrez le port d'ecoute (entre 5000 et 5050): \n");
		std::cin >> portInt;
		if (intDigit(portInt) == 4) {
			itoa(portInt, port, 10);
		}
		else {
			validPort = false;
		}
		if (portInt < 5000 || portInt > 5050) {
			printf("Erreur: le port doit etre entre 5000 et 5050, veuillez reessayer. \n");
			validPort = false;
		}
		std::cin.clear();
		std::cin.ignore(10000, '\n');
	} while (!validPort);

	//récupération du nom d'utilisateur
	char username[80];
	printf("Entrez votre nom d'utilisateur: \n");
	gets_s(username);

	//récupération du mot de passe (le mot de passe ne s'affiche pas à la console)
	SetStdinEcho(false);
	char password[80];
	printf("Entrez votre mot de passe: \n");
	gets_s(password);
	SetStdinEcho(true);


	// getaddrinfo obtient l'adresse IP du host donné
    iResult = getaddrinfo(host, port, &hints, &result);
    if ( iResult != 0 ) {
        printf("Erreur de getaddrinfo: %d\n", iResult);
        WSACleanup();
        return 1;
    }
	//---------------------------------------------------------------------		
	//On parcours les adresses retournees jusqu'a trouver la premiere adresse IPV4
	while((result != NULL) &&(result->ai_family!=AF_INET))   
			 result = result->ai_next; 

//	if ((result != NULL) &&(result->ai_family==AF_INET)) result = result->ai_next;  
	
	//-----------------------------------------
	if (((result == NULL) ||(result->ai_family!=AF_INET))) {
		freeaddrinfo(result);
		printf("Impossible de recuperer la bonne adresse\n\n");
        WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();
        return 1;
	}

	sockaddr_in *adresse;
	adresse=(struct sockaddr_in *) result->ai_addr;
	//----------------------------------------------------
	printf("Adresse trouvee pour le serveur %s : %s\n\n", host,inet_ntoa(adresse->sin_addr));
	printf("Tentative de connexion au serveur %s avec le port %s\n\n", inet_ntoa(adresse->sin_addr),port);
	
	// On va se connecter au serveur en utilisant l'adresse qui se trouve dans
	// la variable result.
	iResult = connect(leSocket, result->ai_addr, (int)(result->ai_addrlen));
	if (iResult == SOCKET_ERROR) {
        printf("Impossible de se connecter au serveur %s sur le port %s\n\n", inet_ntoa(adresse->sin_addr),port);
        freeaddrinfo(result);
        WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();
        return 1;
	}


	//Envoi du nom d'utilisateur et mot de passe
	send(leSocket, username, 80, 0);
	send(leSocket, password, 80, 0);

	//Validation du login
	char loginCode[1]; //0 si refusé, 1 si accepté
	recv(leSocket, loginCode, 1, 0);
	if (loginCode[0] == '1') {
		printf("Connecte au serveur %s:%s\n\n", host, port);
		freeaddrinfo(result);
		printf("Vous pouvez commencer la discussion\n");

		//Création d'un thread pour handle la réception des messages
		DWORD nThreadId;
		HANDLE receiveHandle = CreateThread(0, 0, recvMessage, (void*)&leSocket, 0, &nThreadId);

		//Envoi de messages
		std::string sMotEnvoye;
		do {
			getline(std::cin, sMotEnvoye);
			if (sMotEnvoye.size() > 200)
				printf("Erreur: Message trop long. Veuillez en envoyer un plus petit.");
		} while (sMotEnvoye.size() > 200);
		strcpy(motEnvoye, sMotEnvoye.c_str());
		while (strlen(motEnvoye) != 0) 
		{
			//-----------------------------
			// Envoyer le mot au serveur
			iResult = send(leSocket, motEnvoye, strlen(motEnvoye), 0);
			if (iResult == SOCKET_ERROR) {
				printf("Erreur du send: %d\n", WSAGetLastError());
				closesocket(leSocket);
				WSACleanup();
				printf("Appuyez une touche pour finir\n");
				getchar();

				return 1;
			}
			if (motEnvoye != NULL)
				memset(motEnvoye, 0, sizeof(motEnvoye));

			do {
				getline(std::cin, sMotEnvoye);
				if (sMotEnvoye.size() > 200)
					printf("Erreur: Message trop long. Veuillez en envoyer un plus petit.\n");
			} while (sMotEnvoye.size() > 200);
			strcpy(motEnvoye, sMotEnvoye.c_str());
		}
		// cleanup
		iResult = send(leSocket, "___DISCONNECT___", 16, 0);
		if (iResult == SOCKET_ERROR) {
			printf("Erreur du send: %d\n", WSAGetLastError());
			closesocket(leSocket);
			WSACleanup();
			printf("Appuyez une touche pour finir\n");
			getchar();

			return 1;
		}
		TerminateThread(receiveHandle, 0);
		closesocket(leSocket);
		WSACleanup();

		printf("Appuyez une touche pour finir\n");
		getchar();
		return 0;
	} else {
		//login invalide, on ferme le programme
		printf("Erreur dans la saisie du mot de passe\n");
		printf("Appuyez une touche pour finir\n");
		getchar();
		return 0;
	}
}

//Handler pour la réception des messages
DWORD WINAPI recvMessage(void* s) {
	SOCKET* leSocket = static_cast<SOCKET*>(s);
	char motRecu[330];
	while (true) {
		recv(*leSocket, motRecu, sizeof(motRecu) / sizeof(motRecu[0]), 0);
		printf("%s\n", motRecu);
	}
}

//Détermine le nombre de caractères dans un int
int intDigit(int i) {
	int ndigit = 0;
	i = abs(i);
	return (int)log10((double)i) + 1;
}