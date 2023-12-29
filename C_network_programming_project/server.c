#include <stdio.h>

#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>

#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <pthread.h>

int clientCount;
int* clientList;
struct array {
	int length;
	unsigned char* data;
	int size;
};
struct socketUsername {
	int sock;
	char* username;
};	


struct array make_WLCM(){
	struct array ret;
	unsigned char msg_type = 3;
	unsigned char version = 0;
	ret.data = calloc(2, sizeof(char));
	ret.size=2;
	ret.length=2;
	ret.data[0] = msg_type;
	ret.data[1] = version;
	return ret;
}



void broadcastMESG(struct array MESG){
	for(int i=0;i<clientCount;i++){		
		int bs = send(clientList[i],MESG.data,MESG.length,MSG_WAITALL);
	}
}

void removeClient(int csock){
	int num;
	for(int i=0; i<clientCount; i++){
		if(clientList[i] == csock){
			for(int j=i;j<clientCount;j++){
				clientList[i] = clientList[i+1];
			}
		}
	}
	clientList[clientCount]=0;
	clientCount--;	
}



//listen for messages thread
void *listenForMessages(void* userData){
        int running = 1;
	unsigned char* buf;
	unsigned char rcvd_msg_type;
	struct socketUsername info;
	struct socketUsername *infoptr = (struct socketUsername *)userData;
	info = *infoptr;
	int csock = info.sock;
	char* username = info.username;
	char* rcvdmsg;

	//while no BBYE messages have been sent
        while(running!=0){
        buf = malloc(sizeof(char)*4);
        recv(csock,buf,4,MSG_WAITALL);
        rcvd_msg_type = buf[0];
	
               	if(rcvd_msg_type == 0){
			removeClient(csock);
			running = 0;
		}

		//if message is MESG type, interpret and send back to clients
                 if(rcvd_msg_type == 4){
			//grab length values
			unsigned char namelen = buf[1];
			unsigned short msglen = buf[2];
			//grab timestamp
			buf = malloc(sizeof(char)*4);
			recv(csock,buf,4,MSG_WAITALL);
			int tmstmp = buf[0];
			free(buf);
			//grab username
			buf = malloc(sizeof(char)*namelen);
			recv(csock,buf,namelen,MSG_WAITALL);
			free(buf);
			//grab message
			buf = malloc(sizeof(char)*msglen);
			recv(csock,buf,msglen,MSG_WAITALL);
			char* rcvdmsg = malloc(sizeof(char)*msglen);
			for(int i =0; i<msglen;i++){
				rcvdmsg[i] = buf[i];
			}
			rcvdmsg[msglen] = '\0';
			free(buf);
			//print all info
			printf("At %d, %s said %s \n",tmstmp,username,rcvdmsg);
			//make MESG array
			struct array MESG;
			MESG.data = malloc(sizeof(char)*(8+namelen+msglen));
			MESG.data[0] = 4;
			MESG.data[1] = namelen;
			MESG.data[2] = msglen;
			MESG.data[4] = tmstmp;
			for(int i =0; i< namelen; i++){
				MESG.data[i+8] = username[i];
			}
			for(int i =0; i< msglen; i++){
                                MESG.data[i+8+namelen] = rcvdmsg[i];
                        }
			MESG.length = 8+namelen+msglen;
			// send message to all online users
			broadcastMESG(MESG);
			free(MESG.data);
		}
                

        }
	 printf("Bye %s!\n",username);
        close(csock);
	pthread_cancel(pthread_self());
        }

int main (int argc, char** argv){
	//declared variables
	unsigned short server_port;
	struct stat stats;
	int err;

	//check the input is long enough, throw error if not
	 if(argc<2) {
        printf("Usage: %s <port>\n",argv[0]);
        return -1;
    	}

	// Did we get a port number
    	char* eptr;
    	server_port = (unsigned short)strtol(argv[1], &eptr, 10);
    	if(*eptr != 0 || server_port<1024 || server_port>65535) {
        	printf("Invalid port: %s\n",server_port);
        	return -1;
    	}
	
	// Declare bookkeeping for end points
    	struct sockaddr_in client_addr;
    	struct sockaddr_in server_addr;
    	socklen_t addr_len = sizeof(struct sockaddr_in);

    	int sock;

    	int ret = -1;
	
	 // Filling in the information for the socket
    memset(&server_addr, 0, addr_len);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(0x7F000001);
    server_addr.sin_port   = htons(server_port);

    	 // Get a socket that can be connected to a port
    	sock = socket(AF_INET, SOCK_STREAM, 0);
    	if(sock<0) {
        	perror("Could not get socket from OS");
        	return -1;
    }

 	// Actually connect to the port
    	err = bind(sock, (struct sockaddr*)&server_addr, addr_len);
    	if(err<0) {
        	perror("Binding to the socket failed");
        	close(sock);
        return -1;
    	}


	 // Make my program listen for new connections via the socket
    	err = listen(sock, 10);
    	if(err<0) {
        	perror("Can't listen on the socket");
        	close(sock);
        	return -1;
   		 }

	//server ready for clients
	int rcvd_BBYE =1;
	char* username;
	char* buf = malloc(3*sizeof(char));
	struct array sendWLCM = make_WLCM();
	clientCount =0;
	clientList = malloc(100*sizeof(int));


	while(rcvd_BBYE!=0){
	addr_len = sizeof(struct sockaddr_in);
	int csock =accept(sock, (struct sockaddr*)&client_addr,&addr_len);
	unsigned short cport = ntohs(client_addr.sin_port);
	printf("Connection from %hu\n",cport);
	
	//recv helo message
	 recv(csock, buf,3, MSG_WAITALL);
        unsigned char rcvd_msg_type = buf[0];
        
        if(rcvd_msg_type == 2){
                int namelen = buf[2];
		free(buf);
		buf = malloc(namelen*sizeof(char));
		recv(csock,buf,namelen,MSG_WAITALL);
                username = malloc(namelen*sizeof(char));
                for(int i =0; i<namelen;i++){
                        username[i] = buf[i];
                }
		username[namelen] = '\0';
                printf("%s has joined the server \n",username);
        }	


	//send wlcm message
	int slen = sendWLCM.length;
	int br;
	int bs = send(csock, sendWLCM.data,slen,0);
	
	//setup thread to listen to client
	
	clientList[clientCount] = csock;
	struct socketUsername userData;
	userData.sock = csock;
	userData.username = username;
	pthread_t listen[clientCount];
	pthread_create(&listen[clientCount], NULL, listenForMessages,&userData);
	clientCount++;
	}
	free(username);
	free(buf);
	}
