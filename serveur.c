#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#define SMAX 120
#define PORT_UDP 2000
#define PORT_TCP 2001

char * T_clients[32];   // T_clients contient le nom des machines encore connectées au serveur;
int nb_client;

void print_time() {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    printf("[%02d:%02d:%02d] ", tm->tm_hour, tm->tm_min, tm->tm_sec);
}

// Fonction qui sera très souvent utilisé pour quitter le programme et afficher l'erreur correspondante
void FATAL(char * message) {
    print_time();
    printf("[ERREUR] Problème réseau : %s\n", message);
    perror("Détail système");
    exit(1);
}

void serveur(int sack_diffuseur, struct sockaddr_in * pserv_udp, int sack_recepteur, struct sockaddr_in * pserv_tcp)  {

    int cc, lg, recepteur;
    char msg[SMAX];
    struct hostent * hp;

    lg = sizeof(*pserv_tcp);

    // Initialise le tableau contenant les clients actuellement connectés au serveur
    nb_client = 0;  // Initialise par défaut 0 client
    for(int i = 0; i < 32; i++) T_clients[i] = NULL;    // Par défaut à NULL car 0 client 

    fd_set fdread;

    while(1) {

        FD_ZERO(&fdread);
        FD_SET(sack_diffuseur, &fdread);
        FD_SET(sack_recepteur, &fdread);

        for(int i = 2; i < 32; i++) {
            if(T_clients[i] != NULL) FD_SET(i, &fdread);
        }

        print_time();
        printf("[ATTENTE] En attente d'un message du diffuseur ou d'une connexion d'un récepteur...\n");
        cc = select(32, &fdread, NULL, NULL, NULL);

        for(int i = 2; i < 32; i++) {

            if(T_clients[i] != NULL && FD_ISSET(i, &fdread)) {

                cc = read(i, msg, SMAX);

                if(cc == 0) {

                    print_time();
                    printf("[DECONNEXION] Le récepteur %s s'est déconnecté.\n", T_clients[i]);

                    T_clients[i] = NULL;
                    nb_client--;

                    close(i);

                    print_time();
                    printf("[INFO] Descripteur de la socket fermée : %d\n", i);
                        
                    print_time();
                    printf("[INFO] Nombre de clients restants : %d\n\n", nb_client);

                }

            }

        }

        if(FD_ISSET(sack_diffuseur, &fdread)) {

            cc = recvfrom(sack_diffuseur, msg, SMAX, 0, (struct sockaddr *) pserv_udp, &lg);  // Attend le message envoyé par le diffuseur 

            cc = sendto(sack_diffuseur, msg, strlen(msg), 0, (struct sockaddr *) pserv_udp, sizeof(*pserv_udp));   // Envoyer le message pour confirmer
            if(cc == -1) FATAL("sendto");

            for(int i = 2; i < 32; i++) {

                if(T_clients[i] != NULL) {

                    cc = write(i, msg, strlen(msg));    // Envoi du message au récepteur

                    if(cc == -1) {  // Déconnexion de l'utilisateur si l'envoi du message n'a pas fonctionné

                        print_time();
                        printf("[DECONNEXION] Le récepteur %s s'est déconnecté.\n", T_clients[i]);

                        T_clients[i] = NULL;
                        nb_client--;

                        close(i);

                        print_time();
                        printf("[INFO] Descripteur de la socket fermée : %d\n", i);
                        
                        print_time();
                        printf("[INFO] Nombre de clients restants : %d\n\n", nb_client);

                    }

                    else {
                        print_time();
                        printf("[ENVOI] Message envoyé au récepteur : %s (socket %d)\n", T_clients[i], i);   // Affiche le message reçu pour confirmation
                    }

                }

                if(i == 31) printf("\n");

            }

        }

        if(FD_ISSET(sack_recepteur, &fdread)) {

            lg = sizeof(*pserv_tcp);
            recepteur = accept(sack_recepteur, (struct sockaddr *) pserv_tcp, (socklen_t *) &lg);
            if(recepteur == -1) FATAL("accept");

            hp = gethostbyaddr(&(pserv_tcp->sin_addr), sizeof(struct in_addr), AF_INET);

            // Rajoute le client dans le tableau, on ajoute le client à l'indice du descripteur de fichier, avec comme valeur le nom du client
            T_clients[recepteur] = hp->h_name;
            nb_client++;

            print_time();
            printf("[CONNEXION] Nouveau récepteur connecté : %s\n", T_clients[recepteur]);

            print_time();
            printf("[INFO] Descripteur de la nouvelle socket : %d\n", recepteur);
            
            print_time();
            printf("[INFO] Nombre de clients connectés : %d\n\n", nb_client);

        }

    }
    

}

void main() {

    char nom[SMAX]; 
    int cc, sock_diffuseur, sock_recepteurs;
    struct hostent * s;
    struct sockaddr_in serv_tcp, serv_udp;
    time_t t = time(NULL);

    print_time();
    printf("[INFO] Initialisation du serveur...\n");

    // Récupération du nom du serveur dans le buffer nom
    cc = gethostname(nom, sizeof(nom));
    if(cc == -1) FATAL("gethostname");  // Toujours tester pour éviter d'accumuler les erreurs

    // Récupération des informations du serveur à partir de son hostname (stocké dans le buffer nom)
    s = gethostbyname(nom);
    if(s == NULL) FATAL("gethostbyname");

    // Structure du serveur UDP pour la communication avec le diffuseur
    serv_udp.sin_family = AF_INET;  // Domaine de la socket Unix (UDP)
    serv_udp.sin_port = htons(PORT_UDP);    // Port utilisé pour la communication sur la socket
    print_time();
    printf("[INFO] Port diffuseur (UDP) : %d\n", PORT_UDP);
    bcopy(s->h_addr, (char *) &serv_udp.sin_addr, s->h_length); // Adresse IP utilisée pour la communication sur la socket
    sock_diffuseur = socket(AF_INET, SOCK_DGRAM, 0);  // Création de la socket
    cc = bind(sock_diffuseur, (struct sockaddr *) &serv_udp, sizeof(serv_udp));
    if(cc == -1) FATAL("bind udp"); // Erreur à l'attachement

    // Structure du serveur TCP pour la communication avec les récepteurs
    serv_tcp.sin_family = AF_INET;  // Domaine de la socket Internet (TCP/IP)
    serv_tcp.sin_port = htons(PORT_TCP);    // Port utilisé pour la communication sur la socket
    print_time();
    printf("[INFO] Port récepteurs (TCP) : %d\n", PORT_TCP);
    bcopy(s->h_addr, (char *) &serv_tcp.sin_addr, s->h_length); // Adresse IP utilisée pour la communication sur la socket
    sock_recepteurs = socket(AF_INET, SOCK_STREAM, 0);  // Création de la socket
    cc = bind(sock_recepteurs, (struct sockaddr *) &serv_tcp, sizeof(serv_tcp));
    if(cc == -1) FATAL("bind tcp"); // Erreur à l'attachement
    cc = listen(sock_recepteurs, 5); // 5 pour "combien de connexions en simultannées le serveur peut avoir"
    if(cc == -1) FATAL("listen tcp");

    print_time();
    printf("[INFO] Serveur prêt et en attente d'évènements...\n\n");

    serveur(sock_diffuseur, &serv_udp, sock_recepteurs, &serv_tcp);

    // Termine les connexions en cours
    close(sock_diffuseur);
    close(sock_recepteurs);

}