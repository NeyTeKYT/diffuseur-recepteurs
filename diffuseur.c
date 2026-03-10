#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#define SMAX 120
#define PORT 2000   // Port pour le socket UDP

void print_time() {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    printf("[%02d:%02d:%02d] ", tm->tm_hour, tm->tm_min, tm->tm_sec);
}

// Fonction qui sera très souvent utilisé pour quitter le programme et afficher l'erreur correspondante
void FATAL(char * message) {
    print_time();
    printf("\nERREUR] Problème réseau : %s\n", message);
    perror("Détail système");
    exit(1);
}

void diffuseur(int sack, struct sockaddr_in * pserv) {

    int cc, lg;
    char msg[SMAX];

    while(1) {  // Tant que l'utilisateur ne s'est pas déconnecté = tant que le programme n'a pas été volontairement arrêté

        print_time();
        printf("[SAISIE] Entrez un message à diffuser : ");
        scanf("%s", msg);   // Lire une chaîne de caractères depuis le clavier et mettre dans msg

        cc = sendto(sack, msg, SMAX, 0, (struct sockaddr *) pserv, sizeof(*pserv));   // Envoyer le message 
        if(cc == -1) FATAL("sendto");

        print_time();
        printf("[ENVOI] Message envoyé au serveur : \"%s\"\n", msg);

        print_time();
        printf("[ATTENTE] En attente de confirmation du serveur...\n");

        lg = sizeof(* pserv);
        cc = recvfrom(sack, msg, SMAX, 0, (struct sockaddr *) pserv, &lg);  // Attend la réponse 

        print_time();
        printf("[CONFIRMATION] Le serveur a bien reçu le message : \"%s\"\n\n", msg);   // Affiche le message reçu pour confirmation

    }

}

int main(int argc, char * argv[]) {

    struct hostent * serveur;
    struct sockaddr_in serv;
    int sock;

    // 1 seul argument à l'appel du programme : le nom du serveur
    if(argc != 2) FATAL("Nombre d'arguments incorrect");

    print_time();
    printf("[INFO] Initialisation du diffuseur...\n");

    // Récupération des informations du serveur à partir de son hostname
    serveur = gethostbyname(argv[1]);
    if(serveur == NULL) FATAL("gethostbyname");  // Toujours tester pour éviter d'accumuler les erreurs

    // Structure du serveur
    serv.sin_family = AF_INET;  // Domaine de la socket Internet (TCP/IP)
    serv.sin_port = htons(PORT);  // Port utilisé pour la communication sur la socket

    print_time();
    printf("[INFO] Port diffuseur (UDP) : %d\n", PORT);
    
    bcopy(serveur->h_addr, (char *) & serv.sin_addr, serveur->h_length);    // Adresse IP utilisée pour la communication sur la socket
    // serveur->h_addr = serveur->h_addr_list[0]

    sock = socket(AF_INET, SOCK_DGRAM, 0);  // Création de la socket entre le diffuseur et le serveur

    print_time();
    printf("[INFO] Diffuseur prêt à envoyer des messages...\n\n");

    diffuseur(sock, &serv);

    close(sock);    // Termine la connexion en cours

}
