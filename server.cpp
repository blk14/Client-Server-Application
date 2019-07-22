#include <iostream>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <vector>
#include <iomanip>
#include <map>
#include <bits/stdc++.h>
#include <algorithm>
#include <string>
#include <sstream>

using namespace std;

long double pow (uint8_t p) {
    double ret = 1;
    while (p > 0) {
        ret *= 10;
        p --;
    }
    return ret;
}

class Command {
  public:

    string type;
    string topic;
	int SF;

    void inputParser(char input_buffer[61]) {
        char *pch;
        pch = strtok(input_buffer, " , \n, \0");
        string str(pch);
        this->type = str;
        if (strcmp(pch, "subscribe") == 0) {
            pch = strtok(NULL, " , \n, \0");
            string str1(pch);
            this->topic = str1;
            pch = strtok(NULL, " , \n, \0");
            this->SF = atoi(pch);
        } else {
            pch = strtok(NULL, " , \n, \0");
            string str2(pch);
            this->topic = str2;
        }
    }
};


class Client {
  public:
	char input_buffer[61];
	char client_id[10];
};

class Subscriber {
  public:
    Client client;
    Command command;
    int history_index;
    int socket_subscriber;
    vector<string> string_history;
    vector<string> mytopics;
    int online;
    int SF;


    void setSock(int sock) {
        this->socket_subscriber = sock;
    }
    void setRest(Client &clientz, Command &commandz, int h) {
        this->client = clientz;
        this->command = commandz;
        this->SF = command.SF;
        this->history_index = h;
    }
};

class Topic {
  public:
    string name;
    int tip_date;
    int int_content;
    float short_val_content;
    long double float_content;
    struct sockaddr_in udp_info;
    string string_content;
    map<string, Subscriber> subscribers;
    

    void topicParser (char buffer[1551], struct sockaddr_in udp_info) {
        this->udp_info = udp_info;
        // pasez numele topicului
        char *pch;
        char helper[1551];
        strcpy(helper, buffer);
        pch = strtok(helper, "\0");
        string str(pch);
        name = pch;

        // parsez tipul de date
        uint_least8_t b;
        memcpy(&b, (buffer + 50), sizeof(uint_least8_t));
        this->tip_date = b;    

        // parsez si continutul mesajului

        switch(this->tip_date) {
            case 0:                    // INT
            {
                int sign;
                memcpy(&b, buffer + 51, sizeof(uint_least8_t));
                sign = b;
                uint32_t number;
                memcpy(&number, buffer + 52, sizeof(uint32_t));
                if (sign == 0) {        
                    this->int_content = ntohl(number);
                } else {
                    this->int_content = -ntohl(number);
                }
                break;
            }
            case 1:                  // SHORT_REAL
            {
                uint16_t number1;
                memcpy(&number1, buffer + 51, sizeof(uint16_t));
                this->short_val_content = (ntohs(number1) / 100.0f);
                break;
            }
            case 2:                // FLOAT
            {
                // iau semnul si il convertesc
                int sign2;
                memcpy(&b, buffer + 51, sizeof(uint_least8_t));
                sign2 = b;

                // iau numarul si puterea si convertesc numarul
                uint32_t number2;
                memcpy(&number2, buffer + 52, sizeof(uint32_t));
                uint8_t power;
                memcpy(&power, buffer + 52 + sizeof(uint32_t), sizeof(uint8_t));
                int numar_primit;
                numar_primit = ntohl(number2);

                //convertesc numarul la o precizie mai mare
                long double bufferel = numar_primit;
                if (sign2 == 0) {
                    this->float_content = bufferel / pow(power);
                } else {
                    this->float_content = - (bufferel / pow(power));
                }
                break;
            }
            case 3:                    //String
            {
                // iau stringul delimitat de (buffer + 51) si primul \0
                pch = strtok(buffer + 51, "\0");
                string str3(pch);
                this->string_content = str3;
                break;
            }
        }

    }

// adaug mesajul in vectorul de mesaje din
    void addMessage(Topic topic) {
        // parametrul este folosit doar pentru a lua content-ul
        
        for (map<string, Subscriber>::iterator it = this->subscribers.begin(); it != subscribers.end(); ++it) {
            string buffer;
            buffer += inet_ntoa(udp_info.sin_addr);
            buffer += ":";
            std::string out;
            std::stringstream ss;
            ss << ntohs(udp_info.sin_port);
            out = ss.str();
            buffer += out;
            char bu[20];
            switch(this->tip_date) {
                case 0:
                    buffer += " - ";
                    buffer += this->name;
                    buffer += " - INT - ";
                    memset(bu, 0, 20);
                    snprintf(bu, 20, "%d", topic.int_content);
                    buffer += bu;
                    buffer += "\0";
                    buffer += "\n";
                    it->second.string_history.push_back(buffer);

                    break;
                case 1:
                    buffer += " - ";
                    buffer += this->name;
                    buffer += " - SHORT_REAL - ";
                    memset(bu, 0, 20);
                    snprintf(bu, 20, "%f", topic.short_val_content);
                    buffer += bu;
                    buffer += "\0";
                    buffer += "\n";
                    it->second.string_history.push_back(buffer);

                    break;
                case 2:
                    buffer += " - ";
                    buffer += this->name;
                    buffer += " - FLOAT - ";
                    memset(bu, 0, 20);
                    snprintf(bu, 20, "%Lf", topic.float_content);
                    buffer += bu;
                    buffer += "\0";
                    buffer += "\n";
                    it->second.string_history.push_back(buffer);
                    break;
                case 3:
                    buffer += " - ";
                    buffer += this->name;
                    buffer += " - STRING - ";
                    buffer += topic.string_content;
                    buffer += "\0";
                    buffer += "\n";
                    it->second.string_history.push_back(buffer);
                    break;
            }
        }
    }

    void sendMessage() {
        // trimit mesajele pentru toti subscriberii abonati la acest topic
        for (map<string, Subscriber>::iterator it = this->subscribers.begin(); it != subscribers.end(); ++it) {
            // verific daca e online si daca am ce sa trimit
            if (it->second.command.SF == 0 && it->second.online == 1
            && it->second.string_history.size() > 0) {
                char buff[it->second.string_history[it->second.string_history.size() - 1].size()];
                strcpy(buff, it->second.string_history[it->second.string_history.size() - 1].c_str());

                int ret = send(it->second.socket_subscriber, &buff, sizeof(buff), 0);
                it->second.string_history.erase(it->second.string_history.begin(),
                                                it->second.string_history.end());
                continue;
            }

            if(it->second.command.SF == 1 && it->second.online == 1
            && it->second.string_history.size() > 0) {
                int ok = 0;
                for (int i = it->second.history_index; i < it->second.string_history.size(); i++) {

                    char buff[it->second.string_history[i].size()];
                    strcpy(buff, it->second.string_history[i].c_str());
                    int ret = send(it->second.socket_subscriber, &buff, sizeof(buff), 0);
                    if (ret <= 0) {
                        ok = 0;
                        break;
                    } else {
                        ok = 1;
                    }
                }
                if (ok == 1) {
                    it->second.history_index = 0;
                    it->second.string_history.erase(it->second.string_history.begin(),
                                                    it->second.string_history.end());
                }
                continue;
            }
                  
        }

    }
};

int main(int argc, char *argv[]) {
    int sock_udp, newsock_udp, sock_tcp, newsock_tcp;
    int portno;
    struct sockaddr_in serv_addr, client_adr;
    struct sockaddr_in udp;
    int ret;
    char udp_buffer[1551];
    Client client;
    char tcp_buffer[71];

    map<string, Topic> topicsList;
    map<int, Subscriber> subscribersList;

    fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		cout << "naspa cu nr de argumente" << "\n";
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

    sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_tcp < 0) {
        printf("open socket tcp error \n");
    }

    sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_udp < 0) {
        printf("open socket udp error \n");
    }

    portno = atoi(argv[1]);
    if (portno == 0) {
        printf("portno init error \n");
    }
        
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sock_udp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
    if (ret < 0) {
        printf("bind error (udp) \n");
    }
    ret = bind(sock_tcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
    if (ret < 0) {
        printf("bind error (tcp) \n");
    }

    ret = listen(sock_tcp, MAX_CLIENTS);
    if (ret < 0) {
        printf("listen error (tcp) \n");
    }

    // se adauga file descriptorii (socketii pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sock_udp, &read_fds);
	FD_SET(sock_tcp, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);     // ADAUG SI STANDARD INPUTUL PENTRU CAZUL "EXIT"

    fdmax = max(sock_tcp, sock_udp);

    while (true) {
        tmp_fds = read_fds;
        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select is dead");

        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {          // cazul in care inchid server-ul
            for (int i = 3; i <= fdmax; i++) {
                if (i != sock_tcp && i != sock_udp) {
                    char exit[] = "exit";
                    ret = send(i, exit, strlen(exit), 0);
                }
            }
            break;
        }
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {

                if (i == sock_tcp) {
                    // a venit o cerere de conexiune pe socketul tcp,
                    // pe care serverul o accepta
                    socklen_t clilen = sizeof(client_adr);
                    newsock_tcp = accept(sock_tcp, (struct sockaddr *) &client_adr, &clilen);
                    DIE(newsock_tcp < 0, "accept");

                    // se adauga noul socket intors de accept() la multimea descriptorilor de citire
                    FD_SET(newsock_tcp, &read_fds);
                    if (newsock_tcp > fdmax) { 
                        fdmax = newsock_tcp;
                    }
                    
                    char id[10];
                    ret = recv(newsock_tcp, id, sizeof(id), 0);
                    DIE(ret < 0, "recv");


                    // verific daca clientul a mai fost conectat si daca da ii schimb socketul
                     int new_client = 1;
                     Subscriber s;
                     for (map<int,Subscriber>::iterator it = subscribersList.begin(); it != subscribersList.end(); it++) {
                         if (strcmp(it->second.client.client_id, id) == 0) {
                             if (it->second.online == 1) {
                                char nn[] = "exit";
                                ret = send(newsock_tcp, nn, strlen(nn), 0);
                                DIE(ret < 0, "recv");
                                break;
                             }
                             // sterg subscriber-ul din map  si il adaug cu noua cheie
                            s = it->second;
                            s.online = 1;
                            s.setSock(newsock_tcp);
                            subscribersList.erase(it->first);
                            subscribersList.insert(pair<int, Subscriber> (newsock_tcp, s));
                            subscribersList[newsock_tcp].setSock(newsock_tcp);

                            // setez acest subscriber ca fiind online sau offline in functie de SF
                            // la toate topicurile la care este abonat
                            // urmand ca dupa trimiterea mesajelor retinute prin SF sa ii pun ca online si pe cei cu sf = 0
                            for (int j = 0; j < subscribersList[newsock_tcp].mytopics.size(); j++) {
                                topicsList[subscribersList[newsock_tcp].mytopics[j]].subscribers[s.client.client_id].setSock(newsock_tcp);
                                if (topicsList[subscribersList[newsock_tcp].mytopics[j]].subscribers[s.client.client_id].SF == 1) {
                                    topicsList[subscribersList[newsock_tcp].mytopics[j]].subscribers[s.client.client_id].online = 1;
                                } else {
                                    topicsList[subscribersList[newsock_tcp].mytopics[j]].subscribers[s.client.client_id].online = 0;
                                    topicsList[subscribersList[newsock_tcp].mytopics[j]].subscribers[s.client.client_id].string_history.clear();
                                }
                                

                            }
                             new_client = 0;
                             break;
                         }
                     }
                     // daca este client nou il adaug in lista
                    if (new_client == 1) {
                        s.setSock(newsock_tcp);
                        s.online = 1;
                        strcpy(s.client.client_id, id);
                        subscribersList.insert(pair<int, Subscriber> (newsock_tcp, s));

                        printf("New Client %s connected from %s:%d.\n",
							id, inet_ntoa(client_adr.sin_addr), ntohs(client_adr.sin_port));
                    } else {

                        // trimit mesaje
                            for (int i = 0; i < subscribersList[newsock_tcp].mytopics.size(); i++) {
                                topicsList[subscribersList[newsock_tcp].mytopics[i]].sendMessage();
                            }
                        // ii pun online si pe cei cu sf = 0
                            for (int j = 0; j < subscribersList[newsock_tcp].mytopics.size(); j++) {
                                topicsList[subscribersList[newsock_tcp].mytopics[j]].subscribers[s.client.client_id].online = 1;
                            
                            }
                            
                    }

                    continue;
                }
//  === UDP ====
                if (i == sock_udp) {
                    // s-a primit ceva pe socket-ul udp (un nou topic / mesaj)
                    memset(udp_buffer, 0, sizeof(udp_buffer));
                    socklen_t len;
                    ret = recvfrom(i, udp_buffer, sizeof(udp_buffer), 0, (struct sockaddr *) &udp, &len);
    				DIE(ret < 0, "recv");

                    // parsez mesajul primit de la udp
                    Topic topic;
                    topic.topicParser(udp_buffer, udp);

                    // daca nu am topicul in map, atunci il inserez;
                    // iar daca il am inseamna ca s-a primit un nou mesaj
                    if (topicsList.count(topic.name) == 0) {
                        topicsList.insert(pair<string, Topic>(topic.name, topic));
                        topicsList[topic.name].addMessage(topic);
                    } else {
                        topicsList[topic.name].addMessage(topic);
                    }
                    topicsList[topic.name].sendMessage();
                    // apoi trimit mesajul la subscriberi;

                    continue;
                }
// ====== TCP =====
            // altfel - am primit o comanda de la un subscriber
                memset(client.input_buffer, 0, 61);
                memset(tcp_buffer, 0, 71);
                ret = recv(i, tcp_buffer, sizeof(tcp_buffer), 0);      // primesc comanda de la subscriber
				DIE(ret < 0, "recv");
                
                if (ret == 0) {
				    // conexiunea s-a inchis
				    printf("Client %s disconnected\n", subscribersList[i].client.client_id);
                                            subscribersList[i].online = 0;
                    for (int j = 0; j < subscribersList[i].mytopics.size(); j++) {
                        topicsList[subscribersList[i].mytopics[j]].subscribers[subscribersList[i].client.client_id].online = 0;

                    }
				    close(i);			
					// se scoate din multimea de citire socketul inchis 
					FD_CLR(i, &read_fds);
                    continue;
                }
                Command command;           // locul unde salvez comanda

                // preaiu datele intr-o structura client
                memcpy(client.input_buffer, tcp_buffer, 61);
                memcpy(client.client_id, tcp_buffer + 61, 10);

                command.inputParser(client.input_buffer);         // o parsez si o pun in clasa Command

                // daca am primit o comanda de subscribe
                // verific daca e valida
                // o adaug 
                if (command.type.compare("subscribe") == 0) {
                    if (topicsList.count(command.topic) == 0) {
                        char de[] = "This topic does not exist\n";
                        ret = send(i, de, strlen(de), 0);
                        DIE(ret < 0, "recv");
                        continue;
                    } else {
                        // verific daca s-a mai abonat
                        int ok = 1;
                        if (topicsList[command.topic].subscribers.count(client.client_id) != 0) {
                            char as[] = "you ar already subscribed \n";
                            ret = send(i, as, strlen(as), 0);
                            DIE(ret < 0, "recv");
                             ok = 0;                       
                            continue;
                        }
                        // daca nu s-a mai abonat la topic il abonez
                        // si ii dau si indexul in history in cazul
                        // in care am nevoie de SF, altfel indexul e -1  
                        if (ok == 1) {
                            int history_index;
                            if (command.SF == 1) {

                                if (topicsList[command.topic].subscribers.count(client.client_id) == 0) {
                                    history_index = 0;
                                } else {
                                    history_index = topicsList[command.topic].subscribers[client.client_id].
                                                                                    string_history.size();
                                }
                            } else {
                                history_index = -1;
                            }
                            // adaug subscriber-ul in lista de subscriberi a topicului primit
                            // subscriber-ul care are deja socket-ul setat
                            subscribersList[i].setRest(client, command, history_index);
                            subscribersList[i].mytopics.push_back(command.topic);

                            topicsList[command.topic].subscribers.
                            insert(pair<string, Subscriber> (client.client_id, subscribersList[i]));
                        }
                    }
                } else {
                    // sunt pe cazul unsubscribe + verific daca cumva nu s-a gresit comanda
                    if (command.type.compare("unsubscribe") != 0) {
                        char ic[] =  "invalid command \n";
                        ret = send(i, ic, strlen(ic), 0);
                        DIE(ret < 0, "recv");
                    } else {
                        // daca comanda e valida sterg subscriber-ul din lista de subscriberi
                        // a topicului respectiv
                        topicsList[command.topic].subscribers.erase(client.client_id);
                    }
                }
                
            }
        }
    }

return 0;
}