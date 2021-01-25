/*
I assume that:
            clients do not enter rooms when create it
            all user inputs on menu starts with "-"
            empty rooms not closing

*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

struct rooms{ //I use linked list structure to store both rooms and clients. All room nodes have clients list
    char* roomName;
    struct rooms* next;
    struct clientsInRoom* clients;
    int privacy; //if 0 public, if 1 private
    char* password;
    int capacity;
};
struct clientsInRoom{
    char* clientName;
    struct clientsInRoom* next;
};


struct client{ //it is for send messages
    int sockno;
};
int clientsSockets[100];
int n = 0;

char* sub;//string(char pointer) to divide and store input& other strings
char* sub2;//string(char pointer) to divide and store input& other strings
char* passControl;//string(char pointer) to control password
char* substr;//string(char pointer) to divide and store input& other strings
char* clname;//string(char pointer) to divide and store client name
struct rooms* head = NULL; //to work on list i should an initial null room

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void addRoom(struct rooms* room, char* name, int privacy, char* password){ //it adds room to end of list
    struct rooms* newRoom = NULL;
    newRoom = (struct rooms*)malloc(sizeof(struct rooms));
    newRoom->roomName = (char *)malloc(sizeof(strlen(name)));
    newRoom->password = (char *)malloc(sizeof(strlen(password)));
    newRoom->clients = NULL;
    newRoom->capacity=0;
    strcpy(newRoom->password,password);
    newRoom->privacy = privacy;
    strcpy(newRoom->roomName,name);
    newRoom->next = NULL;
    struct  rooms* temp = room;
    while(temp->next != NULL)
        temp = temp->next;
    temp->next = newRoom;
}
void removeRoom(struct rooms* room){ // not used
    struct rooms* temp = room;
    if(temp->clients ==NULL){
        if(temp->next!=NULL){
            room = temp->next;
            temp->next = NULL;
        }
        //temp->clients =NULL;
        free(temp->password);
        free(temp->roomName);
        //temp = NULL;
        free(temp);

    }else{
        while(temp->next != NULL){
            if(temp->next->clients == NULL){
                struct rooms* temp2 = temp->next;
                temp->next = temp2->next;
                temp2->next= NULL;
                //temp2->clients=NULL;
                free(temp2->password);
                free(temp2->roomName);
                //temp2=NULL;
                free(temp2);
            }
            temp= temp->next;
        }
    }
}


int addClientToRoom(struct rooms* room, char* roomName, char* clientname){ // add clients to front of list
    struct  rooms* temp = room; //temp room for searching in rooms without changing original head
    while(temp!= NULL &&strcmp(temp->roomName, roomName)!=0){
        temp = temp->next;
    }//finds correct room by name comparison
    if(temp!=NULL&& strcmp(temp->roomName, roomName)==0){// if zero they are equal
        struct clientsInRoom* newClient = NULL;
        newClient = (struct clientsInRoom*)malloc(sizeof(struct clientsInRoom));//creating new client
        newClient->clientName=(char *)malloc(sizeof(strlen(clientname)));
        strcpy(newClient->clientName, clientname);
        newClient->next = temp->clients;// adding head new client to clients in room
        temp->clients = newClient;
        temp->capacity++;
    }
    return temp->privacy;

}

void removeClientFromRoom(struct rooms* room, char* roomName, char* clientname){ //removes specific client from room finds room and client and remove it firstly looks front if it is not iterates to find
    struct  rooms* temp = room; //temp room for searching in rooms without changing original head
    while(temp!=NULL&&strcmp(temp->roomName, roomName)!=0){
        temp = temp->next;
    }//finds correct room by name comparison
    if(temp!= NULL&&strcmp(temp->roomName, roomName)==0){// if zero they are equal by string comparison
        struct clientsInRoom* tempClient = temp->clients;
        if(strcmp(temp->clients->clientName, clientname)==0){
            temp->capacity--;
            temp->clients = tempClient->next;
            free(tempClient->clientName);
            tempClient = NULL;
            free(tempClient);

        }else{
            while(tempClient->next!=NULL&&strcmp(tempClient->next->clientName, clientname)!=0){
                tempClient = tempClient->next;
            }
            if(strcmp(tempClient->next->clientName,clientname)==0){
                struct clientsInRoom* tempClient2 = tempClient->next;
                temp->capacity--;
                tempClient->next = tempClient2->next;
                tempClient2->next = NULL;
                free(tempClient2->clientName);
                tempClient2 =NULL;
                free(tempClient2);

            }
        }
        if(temp->privacy==0 && temp->capacity<1){
           // removeRoom(temp);
        }

    }

}
int passwordControl(struct rooms* room, char* roomName, char* clientname, char* passwordControl){ //checks given password in argument correct or not
    struct rooms* temp = room;
    while(temp!=NULL){
        if(strcmp(temp->roomName,roomName)==0){
            if(strcmp(temp->password, passwordControl)==0){
                return 1;
            }else{
                return 0;
            }
        }
        temp = temp->next;
    }
    return 0;
}





void sendOthers(char *msg, int current){ //sends others
    pthread_mutex_lock(&lock);
    int i = 0;
    while(i<n){
        if(clientsSockets[i]!= current){
            send(clientsSockets[i], msg,strlen(msg),0);
        }
        i++;
    }
    pthread_mutex_unlock(&lock);
}
void sendCurrent(char *msg, int current){ //send message to client who send command
    pthread_mutex_lock(&lock);
    int i = 0;
    while(i<n){
        if(clientsSockets[i]== current){
            send(clientsSockets[i], msg,strlen(msg),0);
        }
        i++;
    }
    pthread_mutex_unlock(&lock);

}
void listRooms(struct rooms* room, int current){// it sends list of all rooms in encoded message as "list:roomname:clientnames"
    char * sublist;
    sublist = (char *)malloc(1024*sizeof(char));
    struct rooms* temp = room;
    strcpy(sublist, "list");
    strcat(sublist, ":\n");
    while(temp !=NULL){
        strcat(sublist, "\t");
        strcat(sublist, temp->roomName);
        strcat(sublist, ":\n");
        struct clientsInRoom* tempClient = temp->clients;
        while (tempClient!=NULL){
            if(temp->privacy==0) {
                strcat(sublist, "\t\t");
                strcat(sublist, tempClient->clientName);
                strcat(sublist, "\n");
            }
            tempClient = tempClient->next;

        }

        temp = temp->next;
    }

    sendCurrent(sublist, current);
    sublist = NULL;
    free(sublist);
}


void *receive(void *socket){ //recieving messages from clients
    struct client cl = *((struct client *)socket);
    char msg[1024];
    int len;
    int j;
    while ((len = recv(cl.sockno,msg,1024,0))>0){
        msg[len] = '\0';
        if(msg[0] == '-'){
            strcpy(sub2, msg);
            sub= strtok(msg,";");
            if(strcmp(sub, "-list")==0){ //if command list it makes listrooms method arguments is commander client to send only him
                listRooms(head, cl.sockno);

            }else if (strcmp(sub,"-create")==0){ //if command is create it creates a room
                if(head==NULL){ // if there is no room list
                    sub = strtok(NULL,";");
                    head = (struct rooms*)malloc(sizeof(struct rooms));
                    head->roomName = (char *)malloc(sizeof(strlen(sub)));
                    strcpy(head->roomName,sub);
                    sub = strtok(NULL,";");
                    head->password = (char *)malloc(sizeof(strlen(sub)));
                    head->capacity=0;
                    strcpy(head->password,sub);
                    head->clients = NULL;
                    head->privacy = 0;
                    head->next = NULL;
                    //memset(msg,'\0', sizeof(msg));
                  //  memset(sub,'\0', sizeof(sub));

                }else{ //if there is goes to addroom with decoding roomname, this command for public rooms
                    sub = strtok(NULL,";");
                    addRoom(head,sub,0,"0");
                 //   memset(msg,'\0', sizeof(msg));
                 //   memset(sub,'\0', sizeof(sub));
                }
            }else if (strcmp(sub,"-pcreate")==0){ //same as above but it is private and takes password(not predefined 0)
                if(head==NULL){
                    sub = strtok(NULL,";");
                    head = (struct rooms*)malloc(sizeof(struct rooms));
                    head->roomName = (char *)malloc(sizeof(strlen(sub)));
                    strcpy(head->roomName,sub);
                    sub = strtok(NULL,";");
                    head->password = (char *)malloc(sizeof(strlen(sub)));
                    strcpy(head->password,sub);
                    head->clients = NULL;
                    head->privacy = 1;
                    head->capacity=0;
                    head->next = NULL;
                 //   memset(msg,'\0', sizeof(msg));
                 //   memset(sub,'\0', sizeof(sub));
                }else{
                    sub = strtok(NULL,";");
                    char* temp = (char *)malloc(sizeof(strlen(sub)));
                    strcpy(temp,sub);
                    sub = strtok(NULL,";");
                    addRoom(head,temp,1,sub);
                    free(temp);
                //    memset(msg,'\0', sizeof(msg));
                //    memset(sub,'\0', sizeof(sub));
                }

            }else if (strcmp(sub,"-enter")==0){//enter room command
                sub = strtok(NULL,";");//decode room name
                strcpy(substr,sub);//room name
                sub = strtok(NULL,";"); //client name
                if(addClientToRoom(head,substr,sub)==1){ //when room is private this method returns 1
                    removeClientFromRoom(head,substr,sub);
                    sendCurrent("Password;",cl.sockno);
                }else{
                    sendCurrent("NOPASSWORD;",cl.sockno); //no password required
                }
                //strcpy(receiveBuf,"");
            }else if(strcmp(sub,"-quit")==0){
                sub = strtok(NULL,";"); //roomname
                strcpy(substr,sub);//room name
                sub = strtok(NULL,";"); //client name
                removeClientFromRoom(head, substr, sub);
            }else if (strcmp(sub,"-enterP")==0){ // entering private, this command created by system not user. when user wants to enter private server returns and client sends this method automatically
                sub = strtok(NULL,";");
                strcpy(substr,sub);//room name
                sub = strtok(NULL,";");
                strcpy(clname,sub);//client name
                sub = strtok(NULL,";");
                strcpy(passControl,sub);//Password
                if(passwordControl(head,substr,clname, passControl)==1){//check passsword
                    addClientToRoom(head,substr,clname);
                    strcpy(sub2,"Added;"); //if correct send added msg
                    strcat(sub2,substr);
                    strcat(sub2,";");
                    sendCurrent(sub2,cl.sockno);
                    //sendCurrent("entered", cl.sockno);
                }else if(passwordControl(head,substr,clname, passControl)==0){ //if false send wrong password
                    //removeClientFromRoom(head,substr,clname);
                    sendCurrent("WPassword;",cl.sockno);
                }
            }

        }else{
        sendOthers(msg,cl.sockno);
       // memset(msg,'\0', sizeof(msg));
        }
    }
    pthread_mutex_lock(&lock);
    for(int i =0; i<n;i++){
        if(clientsSockets[i]==cl.sockno){
            j=i;
            while (j<n-1){
                clientsSockets[j]= clientsSockets[j+1];
                j++;
            }
        }
    }
    n--;
    pthread_mutex_unlock(&lock);

}



int main(){
    int socketmain, socket2; //sockets
    struct sockaddr_in addr,addr2;
    socklen_t addr2_size;
    int port = 3205; //predefined port
    pthread_t toSend, toRecv; //threads
    char msg[1024];
    int len;
    struct client cl;
    char ip[INET_ADDRSTRLEN];

    sub = (char *)malloc(1024*sizeof(char)); //allocation of strings
    substr = (char *)malloc(1024*sizeof(char));
    sub2 = (char *)malloc(1024*sizeof(char));
    passControl = (char *)malloc(1024*sizeof(char));
    clname = (char *)malloc(1024*sizeof(char));

    socketmain = socket(AF_INET, SOCK_STREAM,0); // main socket same as client side arguments
    memset(addr.sin_zero,'\0', sizeof(addr.sin_zero));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port); //set port
    addr.sin_addr.s_addr = INADDR_ANY; //set ip as in address(127.0.0.1)
    addr2_size = sizeof(addr2);

    bind(socketmain,(struct sockaddr *)&addr, sizeof(addr)); //after creation of the socket, bind function binds the socket to the address and port number specified

    listen(socketmain,20); // listens main socket to connect requests of clients(max 20 pending request)

    while(1){
        socket2 = accept(socketmain,(struct sockaddr*)&addr2,&addr2_size); //It extracts the first connection request on the queue of pending connections for the listening socket and connects to it returns socket number
        pthread_mutex_lock(&lock);

        inet_ntop(AF_INET, (struct sockaddr *)&addr2, ip, INET_ADDRSTRLEN);

        cl.sockno = socket2;
        clientsSockets[n] = socket2; //we store all clients address
        n++;
        pthread_create(&toRecv, NULL, receive,&cl); // a receive thread created for receiving msg from client all connected clients have exact one thread. Total receive thread = connected client num
        pthread_mutex_unlock(&lock);

    }

    sub=NULL;
    sub2=NULL;
    passControl= NULL;
    substr=NULL;
    clname=NULL;
    free(sub);
    free(sub2);
    free(passControl);
    free(substr);
    free(clname);

    struct rooms *temp = head;
    struct rooms *tempn;
    while (temp!= NULL){
        struct clientsInRoom* cltemp= temp->clients;
        struct clientsInRoom* cltemp2;
        while (cltemp!=NULL){
            cltemp2=cltemp->next;
            free(cltemp->clientName);
            free(cltemp);
            cltemp=cltemp2;
        }
        temp->clients=NULL;
        tempn= temp->next;
        free(temp->roomName);
        free(temp->password);
        free(temp);
        temp = tempn;
    }
    head = NULL;


return 0;

}