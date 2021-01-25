#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

char* roomName; //string(char pointer) to store room name

char* input; //string(char pointer) to take inputs
char* name; //string(char pointer) to store name of user
int mainSocket; //socket of server
char* sub;//string(char pointer) to divide and store input& other strings
char* sub2;//string(char pointer) to divide and store input& other strings
char* substr;//string(char pointer) to divide and store input& other strings

int lock; //when user enters in a private room system waits for password control
char* passwordControl; //string(char pointer) to send password to control
int wrongpass; //wrong password controller
int counter;
struct sockaddr_in addr2; //socket address
int port; // our predefined port is 3205
int len;
char ip[INET_ADDRSTRLEN];
int create;// create controller
int wrongCounter;










void *receive(void *socketF){ //this method(thread) controls messages from server and others
    int sock2 = *((int *)socketF);
    char msg[1024];
    int len;
    counter = 0;
    while(1) {

        while ((len = recv(sock2, msg, 1024, 0)) > 0) {
            if (msg[0] == 'l' && msg[1] == 'i' && msg[2] == 's' && msg[3] == 't') { //if server sends list of rooms system decode it according to list rules
                msg[len] = '\0';
                printf("%s", msg);
                printf("\nenter any non-empty string to return menu\n");
                fflush(stdout);
                memset(msg, '\0', sizeof(msg));


            } else {
                sub = strtok(msg, ";"); //takes first word of encoded message
                if (roomName != NULL) {

                    if (strcmp(sub, roomName) == 0) { //if a message come from same room (in room)
                        sub = strtok(NULL, "\n\r"); //read till end of message and copy it to sub
                        msg[len] = '\0';
                        printf("%s", sub);
                        printf("\n");
                        fflush(stdout);
                        memset(msg, '\0', sizeof(msg));
                    }
                }
                if (sub != NULL ) {
                    if (strcmp(sub, "Password") == 0 && counter == 0) { //server says user wanna enter a private room and requests a password from user
                        printf("Type password: ");
                        fflush(stdout);
                        fgets(sub2, 1024, stdin);
                        passwordControl = strtok(sub2, "\n\r"); //read password
                        strcpy(substr, "-enterP;"); //decoding message to server as I want to enter a private room mesage is "-enterP;roomname;nameofuser;password;
                        strcat(substr, roomName);

                        strcpy(roomName,"");
                        strcat(substr, ";");
                        strcat(substr, name);
                        strcat(substr, ";");
                        strcat(substr, passwordControl);
                        strcat(substr, ";");
                        counter = 1;
                        write(mainSocket, substr, strlen(substr));

                    }else if (strcmp(sub, "Added") == 0) {//if password correct server sends this encoded message(Added;roomname;)
                        strcpy(roomName,strtok(NULL, ";"));
                        lock = 0;
                        wrongpass=0;

                    } else if (strcmp(sub, "WPassword") == 0) {//if entered password is wrong
                        printf("Wrong Password\n");
                        fflush(stdout);
                        strcpy(roomName,"");
                        wrongpass =1;

                    }else if (strcmp(sub, "NOPASSWORD") == 0) {//if room is public server sends no password required message
                        lock = 0;
                        wrongpass=0;
                    }
                }
            }

        }
    }

}

int main(){
    name = (char*)malloc(100* sizeof(char)); //to work with char pointers as string we should allocate them in memory
    input = (char*)malloc(1024* sizeof(char));
    sub = (char *)malloc(1024*sizeof(char));
    sub2 = (char *)malloc(1024*sizeof(char));
    substr = (char *)malloc(1024*sizeof(char));
    passwordControl = (char *)malloc(1024*sizeof(char));
    roomName = (char *) malloc(1024 * sizeof(char));
    counter =0;
    printf("What is your nickname\n"); //first system asks for nickname
    fgets(name,1024,stdin);
    name = strtok(name,"\n\r"); // fgets get name as "name\r" because of enter. we take without \r


    port = 3205; //as mentioned above predefined port
    pthread_t toSend, toRecv; //thread for receive message
    char msg[1024];
    char res[1124];




    mainSocket = socket(AF_INET, SOCK_STREAM,0); //socket(domain,type,protocol) we use IPv4 domain, tcp(sock stream) and Internet Protocol(IP)
    memset(addr2.sin_zero,'\0', sizeof(addr2.sin_zero));
    addr2.sin_family =AF_INET;
    addr2.sin_port = htons(port);
    addr2.sin_addr.s_addr = INADDR_ANY;

    connect(mainSocket,(struct sockaddr *)&addr2, sizeof(addr2));  //The connect() system call connects the socket of server
    inet_ntop(AF_INET, (struct sockaddr *)&addr2, ip, INET_ADDRSTRLEN);
    pthread_create(&toRecv,NULL,receive,&mainSocket); //creates a thread to receive messages and sends main socket as argument to method of receive

    while(1) {

        printf("\n-list\n-create room_name\n-pcreate room_name\n-enter room_name\n-whoami\n-exit\nSelect: "); //menu
        fflush(stdout);
        create =0;
        fgets(input,1024,stdin); //get menu input
        sub = strtok (input," \r\n");
        if(strcmp(sub,"-list")==0){ // all selections should start with "-" according to selection system works
            len = write(mainSocket, "-list", strlen("-list")); //according to selection system sends commands to server
            counter = 0;
        }else if(strcmp(sub,"-whoami")==0){
            printf("%s\n", name);
            fflush(stdout);
        }else if(strcmp(sub,"-exit")==0){
            name = NULL; //freeing memory
            input =NULL;
            sub = NULL;
            sub2 =NULL;
            substr =NULL;
            passwordControl =NULL;
            roomName = NULL;
            free(passwordControl);
            free(substr);
            free(sub2);
            free(sub);
            free(input);
            free(name);
            free(roomName);


            return 1; // memory

        }else if(strcmp(sub,"-create")==0){
            sub = strtok(NULL, "\n\r");
            strcpy(substr, "-create;"); //sends create;roomname;0(as password);nickname;
            strcat(substr, sub);
            strcpy(sub2,sub);
            strcat(substr, ";");
            strcat(substr, "0");
            strcat(substr, ";");
            strcat(substr, name);
            strcat(substr, ";");
            wrongpass =0;
            create=1;
            len = write(mainSocket, substr, strlen(substr));
          //  memset(sub, '\0', sizeof(sub));
          //  memset(substr, '\0', sizeof(substr));
          /*  strcpy(input, "-enter;");
            strcat(input,sub2);
            strcat(input, "\n");
            sub = strtok (input,";");*/


        }else if(strcmp(sub,"-pcreate")==0){
            sub = strtok(NULL, "\n\r");
            strcpy(substr, "-pcreate;"); //sends create;roomname;password;nickname;
            strcat(substr, sub);
            strcat(substr, ";");
            printf("Enter a password: ");
            fflush(stdout);
            char * password=(char *)malloc(1024*sizeof(char));
            fgets(sub,1024,stdin);
            password = strtok(sub,"\n\r");
            strcat(substr,password);
            strcat(substr, ";");
            strcat(substr, name);
            strcat(substr, ";");
            create=1;


            len = write(mainSocket, substr, strlen(substr));
           // memset(sub, '\0', sizeof(sub));
            //memset(substr, '\0', sizeof(substr));
            password = NULL;
            free(password);


        }
        if(strcmp(sub,"-enter")==0) {
            sub = strtok(NULL, "\n\r");//sends -enter;roomname;username;
            strcpy(substr, "-enter;");
            strcat(substr, sub);
            strcpy(roomName, sub);
            strcat(substr, ";");
            strcat(substr, name);
            strcat(substr, ";");
            wrongpass=0;
            counter=0;
            len = write(mainSocket, substr, strlen(substr));
            lock = 1;
            while (lock && !create) { //lock waits for password control
                if (wrongpass == 1) {
                    lock = 0;
                }
            }
            create = 0;


            if (wrongpass==0){
                printf("You can send messages now, type quit for exit room \n");
                fflush(stdout);
                while (fgets(msg, 1024, stdin) > 0) { //when enter room
                    if (strcmp(msg, "quit\n") == 0) {
                        strcpy(res, "-quit"); //if user enters quit system quits to menu
                        strcat(res, ";");
                        strcat(res, roomName);
                        strcat(res, ";");
                        strcat(res, name);
                        strcat(res, ";");
                        len = write(mainSocket, res, strlen(res));
                        strcpy(roomName, "");
                        //free(roomName);
                        counter = 0;
                        break;
                    } else {
                        strcpy(res, roomName); //sends roomname;name:message for room control we check whether same or different rooms in clients
                        strcat(res, ";");
                        strcat(res, name);
                        strcat(res, ":");
                        strcat(res, msg);
                        len = write(mainSocket, res, strlen(res));


                        //  memset(msg, '\0', sizeof(msg));
                        //memset(res, '\0', sizeof(res));
                    }
                }
        }
        }
        /*  while (fgets(msg, 1024, stdin) > 0) { //when enter room
              strcpy(res, name);
              strcat(res, ":");
              strcat(res, msg);
              len = write(mainSocket, res, strlen(res));

              memset(msg, '\0', sizeof(msg));
              memset(res, '\0', sizeof(res));

          }*/
    }

    name = NULL;
    input =NULL;
    sub = NULL;
    sub2 =NULL;
    substr =NULL;
    roomName = NULL;
    passwordControl =NULL;
    free(passwordControl);
    free(substr);
    free(sub2);
    free(sub);
    free(input);
    free(name);
    free(roomName);


    pthread_join(toRecv,NULL);
    close(mainSocket);

    return 0;
}