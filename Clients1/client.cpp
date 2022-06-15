#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <thread>
#include <fcntl.h>
#include <time.h>
#include <cstdlib>
#include "Timer.h"
#define BUFFER_SIZE 512000 //512Kbytes

using namespace std;


void recv_seq_and_ack(int sockfd, int &seq, int &ack);
void send_seq_and_ack(int sockfd, int seq, int ack);
// Three way handshake 
void threeWayHandShake(int sockfd, string ip, int port);
void sendSNY(int sockfd, int SYNbit, int seq);
void recv_SYNbit_seq_ACKbit_ACKnum(int sockfd, int &SYNbit, int &seq, int &ACKbit, int &ACKnum);
void sendACK(int sockfd, int ACKbit, int ACKnum);


void send_seq_and_ack(int sockfd, int seq, int ack) {
    char s[20];
    char a[20];
    sprintf(s, "%d", seq);
    sprintf(a, "%d", ack);
    send(sockfd, s, 20, 0);
    send(sockfd, a, 20, 0);
}

void recv_seq_and_ack(int sockfd, int &seq, int &ack) {
    char buffer[20];
    int a = recv(sockfd, buffer, 20, 0);
    seq = atoi(buffer);
    int b = recv(sockfd, buffer, 20, 0);
    ack = atoi(buffer);
}

// Three way handshake

void sendSNY(int sockfd, int SYNbit, int seq) {
    char s[2];
    char a[2];
    sprintf(s, "%d", SYNbit);
    sprintf(a, "%d", seq);
    send(sockfd, s, 2, 0);
    send(sockfd, a, 2, 0);
}

void recv_SYNbit_seq_ACKbit_ACKnum(int sockfd, int &SYNbit, int &seq, int &ACKbit, int &ACKnum) {
    char buffer[2];
    recv(sockfd, buffer, 2, 0);
    SYNbit = atoi(buffer);
    recv(sockfd, buffer, 2, 0);
    seq = atoi(buffer);
    recv(sockfd, buffer, 2, 0);
    ACKbit = atoi(buffer);
    recv(sockfd, buffer, 2, 0);
    ACKnum = atoi(buffer);
}

void sendACK(int sockfd, int ACKbit, int ACKnum) {
    char s[2];
    char a[2];
    sprintf(s, "%d", ACKbit);
    sprintf(a, "%d", ACKnum);
    send(sockfd, s, 2, 0);
    send(sockfd, a, 2, 0);
}

void threeWayHandShake(int sockfd, string ip, int port) {
    int recv_SYNbit, recv_seq, recv_ACKbit, recv_ACKnum;
    int SYNbit = 1, seq = 2;
    cout << "=====Start the three-way handshake=====" << endl;
    sendSNY(sockfd, SYNbit, seq);
    cout << "Send a packet(SYN) to " << ip << " : " << port << endl;
    recv_SYNbit_seq_ACKbit_ACKnum(sockfd, recv_SYNbit, recv_seq, recv_ACKbit, recv_ACKnum);
    if (recv_ACKbit == SYNbit && recv_ACKnum == seq + 1) {
        cout << "Receive a packet(SYN/ACK) from " << ip << " : " << port << endl;
        cout << "Send a packet(ACK) to " << ip << " : " << port << endl;
        sendACK(sockfd, recv_ACKbit, recv_ACKnum+1);
    }
    cout << "=====Complete the three-way handshake=====" << endl;
}



void run_with_dupAck_at_4096(int sockfd, string ip, int port, string filename) {
    char buffer[20], *b;
    int seq_num = 0, ack_num = 1;
    int recv_seq = 0, recv_ack = 0, recv_size = 0;
    int last_recv = -1;
    int t = 0;

    send(sockfd, filename.c_str(), 6, 0);
    ofstream file (filename, ios::out|ios::binary);
    cout << "Receive file " << filename << " from " << ip << " : " << port << endl;

    while(recv(sockfd, buffer, 20, 0)>0) {
        recv_size = atoi(buffer);
        b = new char[recv_size];
        recv_seq_and_ack(sockfd, recv_seq, recv_ack);
        recv(sockfd, b, recv_size, 0);
        usleep(500000);
        cout << "\tReceive a packet (seq_num = " << recv_seq << ", ack_num = " << recv_ack << ")" << endl;
        if (recv_seq == 4096 && t < 3) {
            send_seq_and_ack(sockfd, seq_num, ack_num);
            t++;
        }else if (recv_seq == ack_num) {
            seq_num++;
            ack_num+=recv_size;
            send_seq_and_ack(sockfd, seq_num, ack_num);
            if (last_recv != ack_num) {
                last_recv = recv_seq;
                file.write (b, recv_size);
            }
        }else{
            send_seq_and_ack(sockfd, seq_num, ack_num);
        }
        delete[] b;
    }

    cout << "=========File " << filename << " Receive=========" << endl;
    file.close();
} 

void run_with_delay(int sockfd, string ip, int port, string filename) {
    char buffer[20], *b;
    int seq_num = 0, ack_num = 1;
    int recv_seq = 0, recv_ack = 0, recv_size = 0;
    int last_recv = -1;

    send(sockfd, filename.c_str(), 6, 0);
    ofstream file (filename, ios::out|ios::binary);
    cout << "Receive file " << filename << " from " << ip << " : " << port << endl;

    while(recv(sockfd, buffer, 20, 0)>0) {
        recv_size = atoi(buffer);
        b = new char[recv_size];
        recv_seq_and_ack(sockfd, recv_seq, recv_ack);
        recv(sockfd, b, recv_size, 0);
        usleep(500000);
        cout << "\tReceive a packet (seq_num = " << recv_seq << ", ack_num = " << recv_ack << ")" << endl;
        if (recv_seq == ack_num) {
            seq_num++;
            ack_num+=recv_size;
            send_seq_and_ack(sockfd, seq_num, ack_num);
            if (last_recv != ack_num) {
                last_recv = recv_seq;
                file.write (b, recv_size);
            }
        }else{
            send_seq_and_ack(sockfd, seq_num, ack_num);
        }
        delete[] b;
    }

    cout << "=========File " << filename << " Receive=========" << endl;
    file.close();
}


void run_with_loss(int sockfd, string ip, int port, string filename) {
    char buffer[20], *b;
    int seq_num = 0, ack_num = 1;
    int recv_seq = 0, recv_ack = 0, recv_size = 0;
    int last_recv = -1;
    srand( time(NULL) );
    

    send(sockfd, filename.c_str(), 6, 0);
    ofstream file (filename, ios::out|ios::binary);
    cout << "Receive file " << filename << " from " << ip << " : " << port << endl;

    while(recv(sockfd, buffer, 20, 0)>0) {
        int ran = rand() % 10 + 1;
        recv_size = atoi(buffer);
        b = new char[recv_size];
        recv_seq_and_ack(sockfd, recv_seq, recv_ack);
        recv(sockfd, b, recv_size, 0);
        cout << "\tReceive a packet (seq_num = " << recv_seq << ", ack_num = " << recv_ack << ")" << endl;
        if (ran == 1)
            recv_seq -= 1;
        if (recv_seq == ack_num) {
            seq_num++;
            ack_num+=recv_size;
            send_seq_and_ack(sockfd, seq_num, ack_num);
            if (last_recv != ack_num) {
                last_recv = recv_seq;
                file.write (b, recv_size);
            }
        }else{
            cout << "LOSS" << endl;
            send_seq_and_ack(sockfd, seq_num, ack_num);
        }
        delete[] b;
    }

    cout << "=========File " << filename << " Receive=========" << endl;
    file.close();
}

void run_with_normal(int sockfd, string ip, int port, string filename){
    char buffer[20], *b;
    int seq_num = 0, ack_num = 1;
    int recv_seq = 0, recv_ack = 0, recv_size = 0;
    int last_recv = -1;

    send(sockfd, filename.c_str(), 6, 0);
    ofstream file (filename, ios::out|ios::binary);
    cout << "Receive file " << filename << " from " << ip << " : " << port << endl;

    while(recv(sockfd, buffer, 20, 0)>0) {
        recv_size = atoi(buffer);
        b = new char[recv_size];
        recv_seq_and_ack(sockfd, recv_seq, recv_ack);
        recv(sockfd, b, recv_size, 0);
        cout << "\tReceive a packet (seq_num = " << recv_seq << ", ack_num = " << recv_ack << ")" << endl;
        if (recv_seq == ack_num) {
            seq_num++;
            ack_num+=recv_size;
            send_seq_and_ack(sockfd, seq_num, ack_num);
            if (last_recv != ack_num) {
                last_recv = recv_seq;
                file.write (b, recv_size);
            }
        }else{
            send_seq_and_ack(sockfd, seq_num, ack_num);
        }
        delete[] b;
    }

    cout << "=========File " << filename << " Receive=========" << endl;
    file.close();
}

// main

int main(int argc, char *argv[]){
    if (argc >= 5) {
        string ip = argv[1];
        int port = atoi(argv[2]);
        string filename;

        struct sockaddr_in server_addr;

        for (int i = 4; i < argc; i++) {
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if(sockfd < 0) {
                cerr << "[-]Error in socket";
                exit(1);
            }

            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

            if(connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
                cerr << "[-]Error in socket";
                exit(1);
            }

            cout << "[+]Connected to Server." << endl;
            threeWayHandShake(sockfd, ip, port);
        
            run_with_normal(sockfd, ip, port, argv[i]);
            close(sockfd);
        }
    }else if (argc == 3) {
        string ip = argv[1];
        int port = atoi(argv[2]);
        char buf[256];
	string userInput;

        struct sockaddr_in server_addr;

        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0) {
            cerr << "[-]Error in socket";
            exit(1);
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

        if(connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            cerr << "[-]Error in socket";
            exit(1);
        }

        cout << "[+]Connected to Server." << endl;

        do {
            //      Enter lines of text
            cout << "> ";
            getline(cin, userInput);
            send(sockfd, userInput.c_str(), userInput.size() + 1, 0);

            //      Wait for response
            memset(buf, 0, 256);
            int bytesReceived = recv(sockfd, buf, 256, 0);
            if (bytesReceived == -1) {
                cout << "There was an error getting response from server\r\n";
            }else{
                cout << "Server> " << buf << "\r\n";
            }
        } while(true);
        
        close(sockfd);


    }
    return 0;
}
