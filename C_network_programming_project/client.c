#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netinet/in.h>
#include <time.h>
#include <pthread.h>


struct array{
	int length;
	unsigned char* data;
	int size;
};

struct array make_HELO(char* username) {
	struct array ret;
	unsigned char msg_type =2;
	unsigned char version = 0;
	unsigned char namelen = strlen(username);
	ret.length = namelen +3;
	ret.data = malloc(sizeof(char)*namelen+3);
	ret.size = 4;
	ret.data[0] = msg_type;
	ret.data[1] = version;
	ret.data[2] = namelen;
	for(int i =0; i<namelen; i++){
		ret.data[i+3] = username[i];
	}
	return ret;
}

struct array make_MESG(char* username, char* message){
	//declare all variables and data
	struct array ret;
	unsigned char msg_type =4;
	unsigned char namelen = strlen(username);
	short* msglen;
	msglen = malloc(sizeof(short));
	*msglen = strlen(message);
	unsigned char msglen1 = msglen[0];
	unsigned char msglen2 = msglen[1];
	ret.length = namelen+*msglen+8;
	ret.data = malloc(sizeof(char)*ret.length);
	int* tmstmp = malloc(sizeof(int));
	*tmstmp = time(NULL);
	
	//pack data into array
	ret.data[0] = msg_type;
	ret.data[1] = namelen;
	ret.data[2] = msglen1;
	ret.data[3] = msglen2;
		//pack time stamp
		for(int i=0; i<4; i++){
			ret.data[4+i] = tmstmp[i];
		}
		//pack username
		for(int i=0; i<namelen; i++){
                        ret.data[8+i] = username[i];
                }
		for(int i=0; i<*msglen; i++){
			ret.data[namelen+8+i] = message[i];
		}
	free(tmstmp);
	free(msglen);
	return ret;
}


int rcv_MESG(char* buf, int sock){
	recv(sock, buf, 4, MSG_WAITALL);
        unsigned char len = buf[1];
        recv(sock, buf, len, MSG_WAITALL);
        buf[len] = '\0';
        printf("%s%", buf);
return 0;
}

void *listenForMessages(void *serverData){
	int running =1;
	unsigned char* buf;
	unsigned char rcvd_msg_type;
	int sock;
	int *placehold = (int *)serverData;
	sock = *placehold;
	char* rcvdmsg;
	char* username;

	while(running!=0){
        buf = malloc(sizeof(char)*4);
        recv(sock,buf,4,MSG_WAITALL);
        rcvd_msg_type = buf[0];

                //if message is MESG type, interpret and send back to clients
                 if(rcvd_msg_type == 4){
                        //grab length values
                        unsigned char namelen = buf[1];
                        unsigned short msglen = buf[2];
                        //grab timestamp
                        buf = malloc(sizeof(char)*4);
                        recv(sock,buf,4,MSG_WAITALL);
                        int tmstmp = buf[0];
                        //grab username
                        buf = malloc(sizeof(char)*namelen);
                        recv(sock,buf,namelen,MSG_WAITALL);
			char* username = malloc(sizeof(char)*namelen);
			for(int i =0; i<namelen;i++){
				username[i] = buf[i];
			}
                        //grab message
                        buf = malloc(sizeof(char)*msglen);
                        recv(sock,buf,msglen,MSG_WAITALL);
                        char* rcvdmsg = malloc(sizeof(char)*msglen);
                        for(int i =0; i<msglen;i++){
                                rcvdmsg[i] = buf[i];
                        }
                        rcvdmsg[msglen] = '\0';
                        //print all info
                        printf("At %d, %s said %s \n",tmstmp,username,rcvdmsg);

}
}
}

int main(int argc, char** argv) {
	unsigned short server_port;
	int err;
	char* username;
	//return error if not enough inputs (server port not included)
	if(argc<3) {
		printf("Usage: %s <port> <username>\n",argv[0]);
		return -1;
	}
	
	//ensure port given is valid
	char* eptr;
	server_port = (unsigned short) strtol(argv[1], &eptr,10);
	if(*eptr != 0) {
        printf("Invalid port: %s\n",server_port);
        return -1;
    }
	//set username
	strcpy(username,argv[2]);
	//declare some vars for socket
	struct sockaddr_in server_addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);

    	int sock;
	
	// Network info for where to connect to
    memset(&server_addr, 0, addr_len);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(0x7F000001);
    server_addr.sin_port   = htons(server_port);

    // Generate the socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock<0) {
        perror("Could not get socket from OS");
        return -1;
    }

    //connect client to the server
    err = connect(sock, (struct sockaddr*)&server_addr, addr_len);
    if(err<0) {
        perror("Connect to the server failed");
        close(sock);
        return -1;
    }
    // The client is connected to the server

   //upon sntering the server, send HELO message
	struct array send_HELO = make_HELO(username);
	unsigned short cport = htons(server_port);
	int slen = send_HELO.length;
int bs = send(sock, send_HELO.data,slen,0); 

	//allow the client to recieve a WLCM message
	char* buf = malloc(sizeof(char)*2);
	recv(sock, buf,2,MSG_WAITALL);
	unsigned char rcvd_msg_type = buf[0];
	
	if (rcvd_msg_type == 3){
		printf("Welcome\n");
		printf("Type message and enter to send,Q to exit \n");
	}
	char key;
	int running = 1;

	//setup new thread to receive and print messages
	pthread_t listen;
	pthread_create(&listen,NULL,listenForMessages,&sock);	
	while( running != 0){
		//take user input for message
		
		char* message;
		message = malloc(sizeof(char)*1024);
		scanf("%s",message);
		if(message[0] == 'Q'){
			running = 0;
		}else{
		//pack MESG data
		struct array myMESG = make_MESG(username, message);
		int bs = send(sock,myMESG.data,myMESG.length,MSG_WAITALL);
		}
	}
	//send BBYE message and close socket
	free(buf);
	buf = malloc(sizeof(char)); 
	*buf = 0;
	bs = send(sock,buf,1,0);
	free(buf);
	close(sock);
	return 0;
}

