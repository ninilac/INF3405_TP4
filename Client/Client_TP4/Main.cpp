#undef UNICODE

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

// Link avec ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

#ifdef WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

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
    char motEnvoye[10];
	char motRecu[10];
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

	char port[5];
	int portInt;
	do {
		printf("Entrez le port d'ecoute (entre 5000 et 5050): \n");
		gets_s(port);
		sscanf(port, "%d", &portInt);
		if (portInt < 5000 || portInt > 5050) {
			printf("Erreur: le port doit etre entre 5000 et 5050, veuillez reessayer. \n");
		}
	} while (portInt < 5000 || portInt > 5050);

	char username[80];
	printf("Entrez votre nom d'utilisateur: \n");
	gets_s(username);

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

	printf("Connecte au serveur %s:%s\n\n", host, port);
    freeaddrinfo(result);

/*	iResult = send(leSocket, username, 80, 0);
	if (iResult == SOCKET_ERROR) {
		printf("Erreur du send: %d\n", WSAGetLastError());
		closesocket(leSocket);
		WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();

		return 1;
	}

	iResult = send(leSocket, password, 80, 0);
	if (iResult == SOCKET_ERROR) {
		printf("Erreur du send: %d\n", WSAGetLastError());
		closesocket(leSocket);
		WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();

		return 1;
	}

	char validateLogin[1];
	iResult = recv(leSocket, validateLogin, 1, 0);
	if (iResult == SOCKET_ERROR) {
		printf("Erreur du send: %d\n", WSAGetLastError());
		closesocket(leSocket);
		WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();

		return 1;
	}*/

	//----------------------------
	// Demander à l'usager un mot a envoyer au serveur
	printf("Saisir un mot de 7 lettres pour envoyer au serveur: ");
	gets_s(motEnvoye);

	//-----------------------------
    // Envoyer le mot au serveur
    iResult = send(leSocket, motEnvoye, 7, 0 );
    if (iResult == SOCKET_ERROR) {
        printf("Erreur du send: %d\n", WSAGetLastError());
        closesocket(leSocket);
        WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();

        return 1;
    }

    printf("Nombre d'octets envoyes : %ld\n", iResult);

	//------------------------------
	// Maintenant, on va recevoir l' information envoyée par le serveur
	iResult = recv(leSocket, motRecu, 7, 0);
	if (iResult > 0) {
		printf("Nombre d'octets recus: %d\n", iResult);
		motRecu[iResult] = '\0';
		printf("Le mot recu est %*s\n", iResult, motRecu);
	}
    else {
        printf("Erreur de reception : %d\n", WSAGetLastError());
    }

    // cleanup
    closesocket(leSocket);
    WSACleanup();

	printf("Appuyez une touche pour finir\n");
	getchar();
    return 0;
}