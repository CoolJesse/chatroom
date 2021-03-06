//Example code: A simple server side code, which echos back the received message.
//Handle multiple socket connections with select and fd_set on Linux 
#include <stdio.h> 
#include <string.h>   //strlen 
#include <stdlib.h> 
#include <errno.h> //?
#include <unistd.h>   //close 
#include <arpa/inet.h>    //close ?
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros ?
    
#define TRUE   1 
#define FALSE  0 
#define PORT 8888 
    
int main(int argc , char *argv[])  
{  
    int opt = TRUE;  
    int master_socket, addrlen, new_socket, activity, i, valread, sd;  
	int client_socket[30];
	int max_clients = 30;
    int max_sd;  
    struct sockaddr_in address;  
        
    char buffer[1025];  //data buffer of 1K 
        
/**************************************************************set of socket descriptors*************************************************************/ 
/***********************The fd_set data type represents file descriptor sets for the select function. It is actually a bit array*********************/

    fd_set readfds;  
        
/**********************************************************************a message**********************************************************************/
    char const *message = "ECHO Daemon v1.0 \r\n";  
    
/**************************************************initialise all client_socket[] to 0 so not checked************************************************/
    for (i = 0; i < max_clients; i++)  
    {  
        client_socket[i] = 0;  
    }  
        
/*************************************************************create a master socket*****************************************************************/ 
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)  
    {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
    
/**********************set master socket to allow multiple connections , this is just a good habit, it will work without this************************/

    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
          sizeof(opt)) < 0 )  
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  
    
/***********************************************************type of socket created*******************************************************************/ 
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons( PORT );  
        
/***************************************************bind the socket to localhost port 8888***********************************************************/
 
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)  
    {  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    printf("Listener on port %d \n", PORT);  
        
/*************************************try to specify maximum of 3 pending connections for the master socket*******************************************/
    if (listen(master_socket, 3) < 0)  
    {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  
        
    //accept the incoming connection 
    addrlen = sizeof(address);  
    puts("Waiting for connections ...");  
	
/***********************************************************Infinite while loop**********************************************************************/
/****************************************************************************************************************************************************/      
    while(TRUE)  
    {  
/****************************************clear the socket set and adds master socket to fd set readfds***********************************************/
 
        FD_ZERO(&readfds); //This macro initializes the file descriptor set readfds to be the empty set.
    
         
        FD_SET(master_socket, &readfds); //This macro adds master_socket to the file descriptor set fd_set readfds.
        max_sd = master_socket;  
          
/*********************************************************add child sockets to set*******************************************************************/ 

        for ( i = 0 ; i < max_clients ; i++)  
        {  
            //socket descriptor 
            sd = client_socket[i];  
                
            //if valid socket descriptor then add to read list 
            if(sd > 0)  
                FD_SET( sd , &readfds);  
                
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  
                max_sd = sd;  
        }  
 
/**********************************wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely*******************************/

/*********************int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)****************************/

		//select() and pselect() allow a program to monitor multiple file descriptors, waiting until one or more of the file descriptors become "ready"
		//for some class of I/O operation (e.g., input possible). A file descriptor is considered ready if it is possible to perform the corresponding 
		//I/O operation (e.g., read(2)) without blocking.
		
		//Three independent sets of file descriptors are watched. Those listed in readfds will be watched to see if characters become available 
		//for reading (more precisely, to see if a read will not block; in particular, a file descriptor is also ready on end-of-file), those in 
		//writefds will be watched to see if a write will not block, and those in exceptfds will be watched for exceptions. On exit, the sets are 
		//modified in place to indicate which file descriptors actually changed status. Each of the three file descriptor sets may be specified as
		//NULL if no file descriptors are to be watched for the corresponding class of events.
		
		//nfds is the highest-numbered file descriptor in any of the three sets, plus 1.
		
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);  
      
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }  
 
/*************************************If something happened on the master socket , then its an incoming connection***********************************/ 		
/******This macro returns a nonzero value (true) if master_socket is a member of the file descriptor set readfds, and zero (false) otherwise.********/

        if (FD_ISSET(master_socket, &readfds)) 
        {  
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  
            {  
                perror("accept");  
                exit(EXIT_FAILURE);  
            }  
            
/***********************************inform user of socket number - used in send and receive commands*************************************************/
		
            printf("New connection , socket fd is %d , ip is : %s , port : %d  \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs
                  (address.sin_port));  
          
/*************************************************send new connection greeting message**************************************************************/ 

            if( send(new_socket, message, strlen(message), 0) != strlen(message) )  
            {  
                perror("send");  
            }  
                
            puts("Welcome message sent successfully");  
			
/******************************************************add new socket to array of sockets***********************************************************/               
            
            for (i = 0; i < max_clients; i++)  
            {  
                //if position is empty 
                if( client_socket[i] == 0 )  
                {  
                    client_socket[i] = new_socket;  
                    printf("Adding to list of sockets as %d\n" , i);  
                        
                    break;  
                }  
            }  
        }  

/**********************************************else its some IO operation on some other socket******************************************************/           
        
        for (i = 0; i < max_clients; i++)  
        {  
            sd = client_socket[i];  
                
            if (FD_ISSET( sd , &readfds))  
            {  
                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = read( sd , buffer, 1024)) == 0)  
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*) &address , (socklen_t*) &addrlen);  
                    printf("Host disconnected , ip %s , port %d \n" , 
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                        
                    //Close the socket and mark as 0 in list for reuse 
                    close( sd );  
                    client_socket[i] = 0;  
                }  
                    
                //Echo back the message that came in 
                else
                {  
                    //set the string terminating NULL byte on the end 
                    //of the data read 
                    buffer[valread] = '\0';  
                    send(sd , buffer , strlen(buffer) , 0 );  
                }  
            }  
        }  
    }  
/****************************************************End of infinite while loop**********************************************************************/
/****************************************************************************************************************************************************/      
    return 0;  
}  
