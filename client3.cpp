#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <iostream>
#include <cstdio>
#include <fstream>

using namespace std;

#define MSGSZ 256
#define SHMSZ 256

#define SERVER_ID 10
#define SERVER_NAME "LG"

//#define CLIENT_ID 3
#define CLIENT_NAME "Nam"

// message queue
struct message {
	long    mtype; //sender
	char    content[MSGSZ]; //content
    int     contentType;
    int     imgId;
    long    receiverId; //receiver
};

int oldStatus[3];
int msqid_server, msqid3, shmid3, shmid_server;
message sbuf, rbuf;
string client[3] = {"Vinh", "Thu", "Nam"};
int *status;

int checkStatusChanged() {
    for(int i=0; i<3; i++) {
        if (oldStatus[i] != status[i]) {
            return 1;
        }
    }
    return 0;
}

void sendMsg() {
    while (status[2]) {
        sbuf.mtype = CLIENT_ID;
        char receiver[100];
        cout << "\nSend to: ";
        cin >> receiver;

        if (strcmp(receiver,"Vinh") == 0) {
            sbuf.receiverId = 1;
        }
        
        if (strcmp(receiver,"Thu") == 0) {
            sbuf.receiverId = 2;
        }

        if (strcmp(receiver,"end") == 0) {
            status[2] = 0;
            cout <<"\nConnection is closed!" << endl;
            exit(0);
        }

        char type[100];
        cout << "Text or Image: ";
        cin >> type;

        if (strcmp(type, "text") == 0) {
            sbuf.contentType = 1;
            sbuf.imgId = 0;
            cout << "Message: ";
            cin >> sbuf.content;
        }

        if (strcmp(type, "image") == 0) {
            sbuf.contentType = 2;
            // shmat to attach to shared memory
            char *str = (char*) shmat(shmid3,(void*)0,0);
            sbuf.imgId = shmid3;

            cout << "Image: ";
            cin >> sbuf.content;

            ifstream image(sbuf.content, ios::in | ios::binary);
            ofstream binary("bin_img_data.txt", ios::out | ios::app);

            char ch;
            while (!image.eof()) {
                ch = image.get();
                *str += ch;
                binary.put(ch);
            }

            image.close();
            binary.close();
        }

        msgsnd(msqid_server, &sbuf, sizeof(sbuf), 0);
    }
}

void receiveMsg() {
    while (status[2]) {
        msgrcv(msqid3, &rbuf, sizeof(rbuf), 0, 0);

        if (rbuf.contentType == 1) {
            if (rbuf.mtype == 1) {
                cout << "\nFrom Vinh: " << rbuf.content << endl;
                cout << "\nSend to: ";
                fflush(stdout);
            }
            if (rbuf.mtype == 2) {
                cout << "\nFrom Thu: " << rbuf.content << endl;
                cout << "\nSend to: ";
                fflush(stdout);
            }
        }
        if (rbuf.contentType == 2) {
            // shmat to attach to shared memory
            char *str = (char*) shmat(rbuf.imgId,(void*)0,0);

            ifstream binary("bin_img_data.txt", ios::in | ios::app | ios::binary);
            ofstream image(rbuf.content, ios::out | ios::app);

            char ch;
            while (!binary.eof()) {
                ch = binary.get();
                image.put(ch);
            }

            image.clear();
            binary.close();

            if (rbuf.mtype == 1) {
                cout << "\nFrom Vinh: " << rbuf.content << endl;
                cout << "\nSend to: ";
                fflush(stdout);
            }
            if (rbuf.mtype == 2) {
                cout << "\nFrom Thu: " << rbuf.content << endl;
                cout << "\nSend to: ";
                fflush(stdout);
            }
        }
    }
}

void manageStatus() {
    while (status[2]) {
        if (checkStatusChanged()) {
            cout << endl;
            for (int i=0; i<3; i++) {
                cout << client[i] << ": " << ((status[i] == 0) ? "Offline" : "Online") << endl;
                oldStatus[i] = status[i];
            }
            cout << "\n\nSend to: ";
            fflush(stdout);
        }
    }
}

int main() {
    key_t key_server, key3;

    // // ftok to generate unique key
    key_server = ftok("vinh.txt", SERVER_ID);
    key3 = ftok("vinh.txt", CLIENT_ID);

    // key_server = 10;
    // key3 = 12;

    // msgget creates a message queue and returns identifier
    msqid_server = msgget(key_server, 0666 | IPC_CREAT );
    msqid3 = msgget(key3, 0666 | IPC_CREAT );

    // shmget returns an identifier in shmid
    shmid_server = shmget(key_server, 1024, 0666 | IPC_CREAT);
    shmid3 = shmget(key3, 1024, 0666 | IPC_CREAT);

    // shmat to attach to shared memory
    status = (int*) shmat(shmid_server,(void*)0,0);

    cout << "\nWelcome Nam to the chat app. Enter 'end' to close the connection." << endl;
    
    status[2] = 1;

    for(int i=0; i<=sizeof(status)/sizeof(int); i++) {
        cout << client[i] << ": " << ((status[i] == 0) ? "Offline" : "Online") << endl;
        oldStatus[i] = status[i];
    }

    thread t1(sendMsg);
    thread t2(receiveMsg);
    thread t3(manageStatus);

    t1.join();
    t2.join();
    t3.join();

    return 0;
}