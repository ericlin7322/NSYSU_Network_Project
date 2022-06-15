#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include "Timer.h"

#define RTT 15000 // 15ms
#define MSS 1024
#define THRESHOLD 8192
#define BUFFER_SIZE 512000 //512 Kbytes
#define PORT "8080"

using namespace std;

enum Congestion {
    slow_start, fast_recovery, congestion_avoidance
};

// init socket
void *get_in_addr(struct sockaddr *sa);

// send and recv
void send_seq_and_ack(int sockfd, int seq, int ack);
void send_data(int sockfd, char data[BUFFER_SIZE], int datalen, int seq, int ack);
void recv_seq_and_ack(int sockfd, int &seq, int &ack);
int check_recv_ack(int sockfd, int seq, int cwnd);

// Three way handshake
void threeWayHandShake(int sockfd);
void recv_SYN(int sockfd, int &SYNbit, int &seq);
void send_SYNACK(int sockfd, int SYNbit, int seq, int ACKbit, int ACKnum);
void recv_ACK(int sockfd, int &ACKbit, int &ACKnum);


// Congestion
void slowStart(int &seq, int &ack, int recv_ack, int &cwnd, int &dupACKcount);
void fastRecovery(int &seq, int &ack, int recv_ack, int &cwnd, int &dupACKcount);
void congestionAvoidance(int &seq, int &ack, int recv_ack, int &cwnd, int &dupACKcount);

void reno(int status, int &seq, int &ack, int &recv_seq, int &recv_ack, int &last_recv, int sent, Congestion &con, int &cwnd, int &ssthresh, int &dupACKcount);
void tahoe(int status, int &seq, int &ack, int &recv_seq, int &recv_ack, int &last_recv, int sent, Congestion &con, int &cwnd, int &ssthresh, int &dupACKcount);

//*****************************************************

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int DNS(char *hostname , char *ip) {
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    if ( (he = gethostbyname(hostname)) == NULL) {
        // get the host info
        herror("gethostbyname");
        return 1;
    }

    addr_list = (struct in_addr **) he->h_addr_list;

    for(i = 0; addr_list[i] != NULL; i++) {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }

    return 1;
}


void send_seq_and_ack(int sockfd, int seq, int ack) {
    char s[20];
    char a[20];
    sprintf(s, "%d", seq);
    sprintf(a, "%d", ack);
    send(sockfd, s, 20, 0);
    send(sockfd, a, 20, 0);
}

void send_data(int sockfd, char data[BUFFER_SIZE], int datalen, int seq, int ack) {
    char a[20];
    sprintf(a, "%d", datalen);
    send(sockfd, a, 20, 0);
    send_seq_and_ack(sockfd, seq, ack);
    send(sockfd, data, datalen, 0);
}

void recv_seq_and_ack(int sockfd, int &seq, int &ack) {
    char buffer[20];
    int a = recv(sockfd, buffer, 20, 0);
    seq = atoi(buffer);
    cout << "\tReceive a packet (seq_num = " << seq;
    int b = recv(sockfd, buffer, 20, 0);
    ack = atoi(buffer);
    cout << ", ack_num = " << buffer << ")" << endl;
}

// Three way handshake

void recv_SYN(int sockfd, int &SYNbit, int &seq) {
    char buffer[2];
    recv(sockfd, buffer, 2, 0);
    SYNbit = atoi(buffer);
    recv(sockfd, buffer, 2, 0);
    seq = atoi(buffer);
}

void send_SYNACK(int sockfd, int SYNbit, int seq, int ACKbit, int ACKnum) {
    char a[2];
    char b[2];
    char c[2];
    char d[2];
    sprintf(a,"%d", SYNbit);
    sprintf(b, "%d", seq);
    sprintf(c,"%d", ACKbit);
    sprintf(d, "%d", ACKnum);
    send(sockfd, a, 2, 0);
    send(sockfd, b, 2, 0);
    send(sockfd, c, 2, 0);
    send(sockfd, d, 2, 0);
}


void recv_ACK(int sockfd, int &ACKbit, int &ACKnum) {
    char buffer[2];
    recv(sockfd, buffer, 2, 0);
    ACKbit = atoi(buffer);
    recv(sockfd, buffer, 2, 0);
    ACKnum = atoi(buffer);
} 

void threeWayHandShake(int sockfd) {
    int seq = 1, ACKbit = 1;
    int recv_SYNbit, recv_seq;
    int recv_ACKbit, recv_ACKnum;
    cout << "=====Start the three-way handshake=====" << endl;
    recv_SYN(sockfd, recv_SYNbit, recv_seq);
    cout << "[+]Receive SYN" << endl;
    send_SYNACK(sockfd, recv_SYNbit, seq, ACKbit, recv_seq + 1);
    cout << "[+]Send SYN/ACK" << endl;
    recv_ACK(sockfd, recv_ACKbit, recv_ACKnum);
    cout << "[+]Receive ACK." << endl;
    if (recv_ACKnum == seq + 1) {
        cout << "[+]Three way handshake success." << endl;
        cout << "=====Complete the three-way handshake=====" << endl;
    }
}

// congestion

void slowStart(int &seq, int &ack, int recv_ack, int &cwnd, int &dupACKcount) {
    cwnd += cwnd;
    dupACKcount = 0;
    // ack ++;
    // seq = recv_ack;
}

void fastRecovery(int &seq, int &ack, int recv_ack, int &cwnd, int &dupACKcount) {
    cwnd += MSS;
}

void congestionAvoidance(int &seq, int &ack, int recv_ack, int &cwnd, int &dupACKcount) {
    cwnd = cwnd + MSS;
    dupACKcount = 0;
    // ack ++;
    // seq = recv_ack;
}

int check(int sockfd, int &seq, int &ack, int &recv_seq, int &recv_ack, int &last_recv, int WS, int cwnd, int &dupACKcount, bool &timeout) {
        recv_seq_and_ack(sockfd, recv_seq, recv_ack);
        if (recv_ack == seq + WS) {
            last_recv = recv_ack;
            seq += WS;
	    ack++;
            return 1;
        }else if (last_recv == recv_ack) {
            last_recv = recv_ack;
            dupACKcount++;
            return -1;
        }else {
            last_recv = recv_ack;
            timeout = true;
            return -2;
        }
}

void reno(int status, int &seq, int &ack, int &recv_seq, int &recv_ack, int &last_recv, int sent, Congestion &con, int &cwnd, int &ssthresh, int &dupACKcount) {
    // status 1 == acked, -1 == duplicateAck, -2 == timeout
    if (status == -1)
        dupACKcount++;
    switch(con) {
        case slow_start:
        if (cwnd >= ssthresh) {
            cout << "======Congestion Avoidance======" << endl;
            con = congestion_avoidance;
            congestionAvoidance(seq, ack, recv_ack, cwnd, dupACKcount);
        }else if(dupACKcount >= 3){
            cout << "======Fast Recovery======" << endl;
            con = fast_recovery;
            ssthresh = cwnd / 2;
            cwnd = ssthresh+3;
            //fastRecovery(seq, ack, recv_ack, cwnd, dupACKcount);
        }else if(status == 1)
            slowStart(seq, ack, recv_ack, cwnd, dupACKcount);
        break;
        case congestion_avoidance:
        if (dupACKcount >= 3) {
            cout << "Receive three duplicated ACKs." << endl;
            cout << "======Fast Recovery======" << endl;
            con = fast_recovery;
            ssthresh = cwnd / 2;
            cwnd = ssthresh+3;
            //fastRecovery(seq, ack, recv_ack, cwnd, dupACKcount);
        }else if (status == -2) {
            cout << "======Slow Start======" << endl;
            ssthresh = cwnd / 2;
            cwnd = 1;
            dupACKcount = 0;
            con = slow_start;
            //slowStart(seq, ack, recv_ack, cwnd, dupACKcount);
        }else if(status == 1)
            congestionAvoidance(seq, ack, recv_ack, cwnd, dupACKcount);
        break;
        case fast_recovery:
            if (status == -2) {
                cout << "======Slow Start======" << endl;
                ssthresh = cwnd / 2;
                cwnd = 1;
                dupACKcount = 0;
                con = slow_start;
                //slowStart(seq, ack, recv_ack, cwnd, dupACKcount);
            }else if(status == 1) {
                cout << "======Congestion Avoidance======" << endl;
                con = congestion_avoidance;
                cwnd = ssthresh;
                dupACKcount = 0;
               // congestionAvoidance(seq, ack, recv_ack, cwnd, dupACKcount);
                ack ++;
                seq = recv_ack;
            }else if(status == -1)
                fastRecovery(seq, ack, recv_ack, cwnd, dupACKcount);
        break;
    }
}



void tahoe(int status, int &seq, int &ack, int &recv_seq, int &recv_ack, int &last_recv, int sent, Congestion &con, int &cwnd, int &ssthresh, int &dupACKcount) {
    // status 1 == acked, -1 == duplicateAck, -2 == timeout
    if (status == -1)
        dupACKcount++;
    switch(con) {
        case slow_start:
            if (cwnd >= ssthresh) {
                cout << "======Congestion Avoidance======" << endl;
                con = congestion_avoidance;
                cwnd = cwnd + MSS;
		dupACKcount = 0;
            }else if (dupACKcount >= 3) {
                cout << "Receive three duplicated ACKs." << endl;
                cout << "======Fast retrasmit======" << endl;
                cout << "======Slow Start======" << endl;
                ssthresh = cwnd / 2;
                cwnd = 1;
                dupACKcount = 0;
            }else if(status == 1)
                cwnd += cwnd;
                dupACKcount = 0;
                ack ++;
                seq = recv_ack;
            break;
        case congestion_avoidance:
            if (dupACKcount >= 3) {
                cout << "Receive three duplicated ACKs." << endl;
                ssthresh = cwnd / 2;
                cwnd = 1;
                dupACKcount = 0;
                cout << "======Fast retrasmit======" << endl;
                cout << "======Slow Start======" << endl;
                con = slow_start;
            }else if(status == 1)
                cwnd = cwnd + MSS;
                dupACKcount = 0;
                ack ++;
                seq = recv_ack;
            break;
        case fast_recovery:
            break;
    }
}

// Run

void run_Reno(int sockfd, char filename[6]) {
    long fsize;
    Congestion con = slow_start;
    int ack_Num = 0, seq_num = 1;
    int cwnd = 1, ssthresh = THRESHOLD, dupACKcount = 0;
    int recv_seq, recv_ack;
    int last_recv = -1;
    int temp_seq = seq_num;
    bool timeout = false;
    string f = filename;
    ifstream file (f, ios::in|ios::binary|ios::ate);
    if (!file)
        cout << "Failed to open file" << endl;
    fsize = file.tellg();
    cout << "File Size: " << fsize << endl;

    do {
        if (seq_num-1 == 0)
            cout << "======Slow Start======" << endl;
        
        if (seq_num-1 + cwnd >= fsize)
            cwnd = fsize - seq_num + 1;

        cout << "cwnd = " << cwnd << ", rwnd = " << fsize - seq_num << ", threshold = " << ssthresh << endl;
        if (cwnd <= 1024) {
            char buffer[cwnd];
            file.seekg (seq_num - 1, ios::beg);
            file.read (buffer, cwnd);
            send_data(sockfd, buffer, cwnd, seq_num, ack_Num);
            cout << "\tSend a packet at : " << seq_num << " byte" << endl;
            int c = check(sockfd, seq_num, ack_Num, recv_seq, recv_ack, last_recv, cwnd, cwnd, dupACKcount, timeout);
            reno(c, seq_num, ack_Num, recv_seq, recv_ack, last_recv, 0, con, cwnd, ssthresh, dupACKcount);
        }else if (cwnd > 1024) {
            int WS = 1024, sent = 0;
            char buffer[1024];
            int c;
            while (sent < cwnd) {
                if (WS + sent > cwnd)
                    WS = cwnd - sent;
                file.seekg (seq_num - 1, ios::beg);
                file.read (buffer, WS);
                send_data(sockfd, buffer, WS, seq_num, ack_Num);
                cout << "\tSend a packet at : " << seq_num << " byte" << endl;
                sent += WS;
                c = check(sockfd, seq_num, ack_Num, recv_seq, recv_ack, last_recv, WS, cwnd, dupACKcount, timeout);
                if (con == fast_recovery && c == 1)
			break;
		if (dupACKcount == 3)
                    break;
            }
            reno(c, seq_num, ack_Num, recv_seq, recv_ack, last_recv, sent, con, cwnd, ssthresh, dupACKcount);

        }
    }while(seq_num-1 < fsize);
    file.close();
}

void run_Tahoe(int sockfd, char filename[6]) {
    long fsize;
    Congestion con = slow_start;
    int ack_Num = 0, seq_num = 1;
    int cwnd = 1, ssthresh = THRESHOLD, dupACKcount = 0;
    int recv_seq, recv_ack;
    int last_recv = -1;
    int temp_seq = seq_num;
    bool timeout = false;
    string f = filename;
    ifstream file (f, ios::in|ios::binary|ios::ate);
    if (!file)
        cout << "Failed to open file" << endl;
    fsize = file.tellg();
    cout << "File Size: " << fsize << endl;

    do {
        if (seq_num-1 == 0)
            cout << "======Slow Start======" << endl;
        
        if (seq_num-1 + cwnd >= fsize)
            cwnd = fsize - seq_num + 1;

        cout << "cwnd = " << cwnd << ", rwnd = " << fsize - seq_num << ", threshold = " << ssthresh << endl;
        if (cwnd <= 1024) {
            char buffer[cwnd];
            file.seekg (seq_num - 1, ios::beg);
            file.read (buffer, cwnd);
            send_data(sockfd, buffer, cwnd, seq_num, ack_Num);
            cout << "\tSend a packet at : " << seq_num << " byte" << endl;
            int c = check(sockfd, seq_num, ack_Num, recv_seq, recv_ack, last_recv, cwnd, cwnd, dupACKcount, timeout);
            tahoe(c, seq_num, ack_Num, recv_seq, recv_ack, last_recv, 0, con, cwnd, ssthresh, dupACKcount);
        }else if (cwnd > 1024) {
            int WS = 1024, sent = 0;
            char buffer[1024];
            int c;
            while (sent < cwnd) {
                if (WS + sent > cwnd)
                    WS = cwnd - sent;
                file.seekg (seq_num - 1, ios::beg);
                file.read (buffer, WS);
                send_data(sockfd, buffer, WS, seq_num, ack_Num);
                cout << "\tSend a packet at : " << seq_num << " byte" << endl;
                sent += WS;
                c = check(sockfd, seq_num, ack_Num, recv_seq, recv_ack, last_recv, WS, cwnd, dupACKcount, timeout);
                if (dupACKcount == 3)
                    break;
            }
            tahoe(c, seq_num, ack_Num, recv_seq, recv_ack, last_recv, sent, con, cwnd, ssthresh, dupACKcount);

        }
    }while(seq_num-1 < fsize);
    file.close();
}

int main(){
    char filename[5];

    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (::bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 100) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;) {
        struct timeval timeout;
        timeout.tv_sec=1;
        timeout.tv_usec=0;
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                    threeWayHandShake(i);
                    if (recv(i, filename, 6, 0) == 6) {
                        cout << "Filename: " << filename << endl;
                        run_Reno(i, filename);
                    }
                    cout << "========= File " << filename << " send =========" << endl;
                    close(i);
                    FD_CLR(i, &master);
                }
            }
        }
    }

    return 0;
}
