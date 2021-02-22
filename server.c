#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 20 //max 20 users in app at the sama time.
#define ROOM_CAPACITY 5 //each group capacity.
#define MAX_ROOMS 10 //max 10 group at the same time.
#define SIZE 2048

//structs
typedef struct
{
    struct sockaddr_in address;
    int sockfd;
    int userid; //id
    char username[32]; //phone number
    int inRoom; //in room or not
} client; //user

typedef struct
{
    client *clientsInRoom[ROOM_CAPACITY]; //users
    char name[32]; //group name
    char pwd[32]; //group password
} room;
//

//global variables.
static int clientCounter = 0;
static int useridCounter = 1;
client *users[MAX_CLIENTS];
room *rooms[MAX_ROOMS];

char MSG_RECEIVE[SIZE];
char MSG_SEND[SIZE];

int *newsock;
//

//send message to only one user which is sender user.
void sendMessageToSender(char *string, int userid)
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (users[i])
        {
            if (users[i]->userid == userid)
            {
                if (send(users[i]->sockfd, string, strlen(string), 0) < 0)
                {
                    perror("ERROR: write to descriptor failed");
                    break;
                }
            }
        }
    }
}

//send message to other users in whatsapp group.
void sendOtherUsers(char *string, int userid)
{
    int index = -1;
    for (int i = 0; i < MAX_ROOMS; i++)
    {
        if (rooms[i])
        {
            for (int j = 0; j < ROOM_CAPACITY; j++)
            {
                if (rooms[i]->clientsInRoom[j])
                {
                    if (rooms[i]->clientsInRoom[j]->userid == userid)
                    {
                        index = i;
                        break;
                    }
                }
            }
        }
    }
    if (index != -1)
    {
        for (int i = 0; i < ROOM_CAPACITY; i++)
        {
            if (rooms[index]->clientsInRoom[i])
            {
                if (rooms[index]->clientsInRoom[i]->userid != userid)
                {
                    if (send(rooms[index]->clientsInRoom[i]->sockfd, string, strlen(string), 0) < 0)
                    {
                        perror("ERROR: Message could not send to other users in room.");
                        break;
                    }
                }
            }
        }
    }
}

// -gcreate
void createRoom(char *groupName, void *client_t)
{
    char message[500];
    char password[16];

    client *user = (client *)client_t;
    if (user->inRoom == 0) // user is already in a group or not ?
    {
        strcpy(message, "Create a password : ");
        sendMessageToSender(message, user->userid);
        int receivePwd = recv(user->sockfd, password, 16, 0); //taking group password from user
        if (receivePwd > 0)
        {
            room *newRoom = (room *)malloc(sizeof(room));
            strcpy(newRoom->name, groupName);
            strcpy(newRoom->pwd, password);
            int index = 0;

            //find empty index for new room.
            for (index = 0; index < MAX_ROOMS; ++index)
            {
                if (!rooms[index])
                {
                    rooms[index] = newRoom;
                    break;
                }
            }
            user->inRoom = 1; //update.
            //add user to room.
            if (rooms[index])
            {
                for (int i = 0; i < ROOM_CAPACITY; i++)
                {
                    if (!rooms[index]->clientsInRoom[i])
                    {
                        rooms[index]->clientsInRoom[i] = user;
                        break;
                    }
                }
            }
            strcpy(message, "\n\t\t You created a group. Welcome to group ");
            strcat(message, newRoom->name);
            strcat(message, "\n");
            sendMessageToSender(message,user->userid);
            printf("User %s created a group with name %s \n", user->username, newRoom->name);
        }
    }
    else
    {
        strcpy(message, "\n You are currently in a group, first you need to leave from this group with ' -exit groupname '\n");
        sendMessageToSender(message, user->userid);
    }
}

// -join
void joinRoom(char *groupName, void *client_t)
{

    client *user = (client *)client_t;
    char message[SIZE];
    char password[16];

    // find room index.
    int index = -1;
    for (int i = 0; i < MAX_ROOMS; i++)
    {
        if (rooms[i])
        {
            if (rooms[i]->name)
            {
                if (strcmp(rooms[i]->name, groupName) == 0)
                {
                    index = i;
                    break;
                }
            }
        }
    }

    // if there is a index.
    if (index != -1)
    {
        strcpy(message, "Please enter the password to join to room : ");
        sendMessageToSender(message, user->userid);
        bzero(message, SIZE);
        int receivePassword = recv(user->sockfd, password, 16, 0); //taking password from user.
        if (receivePassword > 0)
        {
            if (strlen(password) > 0)
            {
                if (strcmp(rooms[index]->pwd, password) == 0)
                {
                    // add user to room
                    user->inRoom = 1;
                    if (rooms[index])
                    {
                        for (int i = 0; i < ROOM_CAPACITY; i++)
                        {
                            if (!rooms[index]->clientsInRoom[i])
                            {
                                rooms[index]->clientsInRoom[i] = user;
                                break;
                            }
                        }
                    } //
                    strcpy(message, "\n\t\t You joined to group ");
                    strcat(message, rooms[index]->name);
                    strcat(message, "\n");
                    sendMessageToSender(message, user->userid);
                    sleep(1);
                    bzero(message, SIZE);
                    strcpy(message, "\n\t\t");
                    strcat(message, user->username);
                    strcat(message, " joined to your group \n");
                    sendOtherUsers(message, user->userid);
                    printf("User %s joined the group %s \n", user->username, rooms[index]->name);
                }
                else
                {
                    bzero(message, SIZE);
                    strcpy(message, "The password is wrong !!");
                    sendMessageToSender(message, user->userid);
                }
            }
        }
    }
    else
    {
        strcpy(message, "There is no group with this name! \n");
        sendMessageToSender(message, user->userid);
    }
}

// -send
void sendMessage(char *body, void *client_t)
{
    client *user = (client *)client_t;
    char message[SIZE];
    //char clearMSG[SIZE];
    if (user->inRoom != -1)
    {
        strcpy(message, "\t\t\t");
        strcat(message, user->username);
        strcat(message, " :");
        strcat(message, body);
        sendOtherUsers(message, user->userid);
        bzero(message, SIZE);
    }
    else
    {
        char err_message[SIZE];
        strcpy(err_message, "You are currently in not any group!!!\n");
        sendMessageToSender(err_message, user->userid);
        bzero(err_message, SIZE);
    }
}

// -exit group_name
void exitGroup(char *groupName, void *client_t)
{
    client *user = (client *)client_t;
    char message[SIZE];

    if (user->inRoom == 1)
    {
        //find room index with userid.
        int index = -1;
        for (int i = 0; i < MAX_ROOMS; i++)
        {
            if (rooms[i])
            {
                for (int j = 0; j < ROOM_CAPACITY; j++)
                {
                    if (rooms[i]->clientsInRoom[j])
                    {
                        if (rooms[i]->clientsInRoom[j]->userid == user->userid)
                        {
                            index = i;
                            break;
                        }
                    }
                }
            }
        }
        //
        if (index != -1)
        {
            char MSG2[SIZE];
            strcpy(MSG2, "\t\t >  ");
            strcat(MSG2, user->username);
            strcat(MSG2, " has left the group\n");
            sendOtherUsers(MSG2, user->userid);

            //remove user from room/group.
            if (strcmp(rooms[index]->name, groupName) == 0)
            {
                int count = 0;
                for (int i = 0; i < ROOM_CAPACITY; i++)
                {
                    if (rooms[index]->clientsInRoom[i])
                    {
                        if (rooms[index]->clientsInRoom[i]->userid == user->userid)
                        {
                            rooms[index]->clientsInRoom[i] = NULL;
                            break;
                        }
                    }
                }
                //
                user->inRoom = 0;
                strcpy(message, "\t\t You left from group\n");
                sendMessageToSender(message, user->userid);
                bzero(message, SIZE);
                printf("User %s left from the group %s \n", user->username, rooms[index]->name);

                //count the user count in the room. if room is empty close the room.
                for (int i = 0; i < ROOM_CAPACITY; i++)
                {
                    if (rooms[index]->clientsInRoom[i])
                    {
                        count++;
                    }
                }
                if (count == 0)
                {
                    rooms[index] = NULL;
                }
                //
            }
            else
            {
                strcpy(message, "\t\t You enter invalid group name !! please try again \n");
                sendMessageToSender(message, user->userid);
            }
        }
    }
    else
    {
        char err_message[SIZE];
        strcpy(err_message, "You are currently in not any group!!!\n");
        sendMessageToSender(err_message, user->userid);
        bzero(err_message, SIZE);
    }
}

void *receiveMessage(void *client_t)
{
    char PHONE[12];
    char COPY[SIZE];
    int splitIndex = 0;
    char *String;
    char *SplittedString[20];

    client *user = (client *)client_t;

    //first take phone number / username from user.
    if (recv(user->sockfd, PHONE, 12, 0) <= 0)
    {
        printf("Didn't enter the name.\n");
    }
    else
    {
        strcpy(user->username, PHONE);
        printf("%s is connected to server ! \n", user->username);
    }

    //infinite loop.
    while (1)
    {
        bzero(MSG_RECEIVE, SIZE);
        int receive = recv(user->sockfd, MSG_RECEIVE, SIZE, 0); //take command from user.

        if (receive > 0)
        {
            //split the command.
            splitIndex = 0;
            strcpy(COPY, MSG_RECEIVE);
            String = strtok(MSG_RECEIVE, " ");
            while (String != NULL)
            {
                SplittedString[splitIndex] = String;
                splitIndex++;
                String = strtok(NULL, "\n");
            }
            //controls:
            if (strcmp(SplittedString[0], "-gcreate") == 0 && SplittedString[1] != NULL)
            {
                createRoom(SplittedString[1], (void *)user);
            }
            else if (strcmp(SplittedString[0], "-join") == 0 && SplittedString[1] != NULL)
            {
                joinRoom(SplittedString[1], (void *)user);
            }
            else if (strcmp(SplittedString[0], "-send") == 0)
            {
                sendMessage(SplittedString[1], (void *)user);
            }
            else if (strcmp(SplittedString[0], "-exit") == 0 && strcmp(COPY, "-exit") != 0 && SplittedString[1] != NULL) //user exit from group
            {
                exitGroup(SplittedString[1], (void *)user);
            }
            else if (strcmp(COPY, "-exit") == 0) //user exit from server.
            {
                for (int i = 0; i < MAX_CLIENTS; ++i)
                {
                    if (users[i])
                    {
                        if (users[i]->userid == user->userid)
                        {
                            users[i] = NULL;
                            break;
                        }
                    }
                }
            }
            else if (strcmp(SplittedString[0], "-whoami") == 0)
            {
                char info[SIZE];
                strcpy(info, "You are : ");
                strcat(info, user->username);
                strcat(info, "\n");
                sendMessageToSender(info, user->userid);
                bzero(info, SIZE);
            }
            else
            {
                //printf("There is no command :");
                puts(COPY);
                //printf("\n");
            }
        }
    }
}

int main(int argc, char *argv[])
{

    int socket_desc, new_socket, c;
    struct sockaddr_in server, client_addr;
    pthread_t roomtid[MAX_ROOMS];
    pthread_t servertid;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0); //listenfd

    if (socket_desc == -1)
    {
        puts("Could not create socket");
        return 1;
    }

    server.sin_family = AF_INET;         //IPv4 Internet protocols
    server.sin_addr.s_addr = INADDR_ANY; //IPv4 local host address
    server.sin_port = htons(3205);

    // Bind
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        puts("Binding failed");
        return 1;
    }
    puts("Socket is binded");

    // Listen
    if (listen(socket_desc, 10) < 0)
    {
        perror("ERROR: Socket listening failed");
        return EXIT_FAILURE;
    }
    printf("=== WELCOME TO THE WHATSAPP CHAT APP SERVER ===\n");
    // Accept and incoming connection
    puts("Waiting for incoming connections...");

    while (1)
    {
        socklen_t c = sizeof(client_addr);
        new_socket = accept(socket_desc, (struct sockaddr *)&client_addr, &c);
        if (new_socket < 0)
        {
            puts("Accept failed");
            return 1;
        }
        puts("Connection accepted");

        /* Client settings */
        client *user = (client *)malloc(sizeof(client));
        user->address = client_addr;
        user->sockfd = new_socket;
        user->userid = useridCounter++;
        user->inRoom = 0;

        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (!users[i])
            {
                users[i] = user;
                break;
            }
        }

        pthread_create(&servertid, NULL, &receiveMessage, (void *)user);
        sleep(1);
    }

    close(socket_desc);
    return EXIT_SUCCESS;
}
