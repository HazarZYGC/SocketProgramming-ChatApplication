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

#define SIZE 2048
#define PORT 3205

//global variables
char MSG [SIZE];
char RECEIVE[SIZE];
char PHONE[12];
int sockfd = 0;
int exitAPP = 0;

//helper function
void str_overwrite_stdout() {
  printf("%s", "> ");
  fflush(stdout);
}

//helper function
void trim_row (char* str, int length) 
{
  int index;
  for (index = 0; index < length; index++) 
  { 
    if (str[index] == '\n') 
    {
      str[index] = '\0';
      break;
    }
  }
}

// send message from client to server.
void sendMessage()
{
    while (1)
    {
        str_overwrite_stdout();
        fgets (MSG, sizeof(MSG), stdin); //input from client
        trim_row(MSG,strlen(MSG));
        if(strlen(MSG)==0) //if input is empty
        {
            printf("Please enter a command with argument" );
        }
        else if(strcmp(MSG, "-exit") == 0) //exit from app
        {
            exitAPP=1;
			break;
        }
        else
        {
            if(send(sockfd , MSG , sizeof(MSG) , 0) < 0) //send to server
            {
                printf("Currently, your message can't be sent" );
            }
              
        }
        bzero(MSG,SIZE); 
    } 
}

void receiveMesage() //receive message from server
{
    while (1) 
    {
		int receive_flag = recv(sockfd, RECEIVE, SIZE, 0);
        if (receive_flag > 0) 
        {
            printf("%s \n", RECEIVE);
            str_overwrite_stdout();
        } 
        else 
        {
			break;
		}
        bzero(RECEIVE,SIZE);
    }
}

int main(int argc, char **argv)
{

    struct sockaddr_in server;

	/* Socket settings */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    int err = connect(sockfd, (struct sockaddr *)&server, sizeof(server)); //connecto to server.

    if (err == -1) 
    {
	    printf("ERROR: connect\n");
	    return EXIT_FAILURE;
	}

    printf("=== WELCOME TO THE WHATSAPP ===\n");
    printf("You are online on Whatsapp\n");   
    printf("1) -gcreate group_name : let you create group\n");   
    printf("*) -gcreate ONLY TAKE PHONE NUMBER DONT USE +\n");   
    printf("2) -join group_name : let you join to group or create private chat\n");   
    printf("3) -send : let you message to the group\n");   
    printf("4) -exit groupname : let you exit the group\n");   
    printf("5) -exit : let you quit the program\n");   
    printf("6) -whoami : let you see your number\n");
    
    while(1)  //taking phone_number (username)
    {
        printf("\n Please enter your phone number (it will be also username): ");
        fgets(PHONE, 12, stdin);
        trim_row(PHONE,strlen(PHONE));
        if(strlen(PHONE) != 0)
        {
            if(send(sockfd, PHONE, 12, 0)<0)
            {
                printf("Currently, your username can't be sent" );
            }
            else break;
        }
        else
        {
            printf("You must enter a phone number (username)" );
        }
    }
    

    pthread_t send_msg_thread;
    if(pthread_create(&send_msg_thread, NULL, (void *) sendMessage, NULL) != 0)
    {
		printf("ERROR: pthread\n");
        return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
    if(pthread_create(&recv_msg_thread, NULL, (void *) receiveMesage, NULL) != 0)
    {
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}
    while (1)
    {
        if(exitAPP)
        {
            printf("\nBye\n");
            break;
        }
	}
    close(sockfd);
	return EXIT_SUCCESS;
}