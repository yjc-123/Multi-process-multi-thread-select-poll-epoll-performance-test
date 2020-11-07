#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
       #include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
 #include <sys/types.h> 
 #include <sys/socket.h>
void handle_sigchld2(int signo)
{
	int mypid;
	while (( mypid = waitpid(-1, NULL, WNOHANG)) > 0){
		 printf("child %d terminated\n", mypid); 
	} 
}

int main(){

	int			listen_fd,client_fd;
	struct sockaddr_in	serv_addr;
	struct sockaddr_in	cli_addr;
	socklen_t		socklen = sizeof(cli_addr);
	char			buf[1024];
	int 			rv;
	listen_fd = socket(AF_INET, SOCK_STREAM,0);
	if(listen_fd < 0){
		printf("creat socket_fd is failed:[%s]\n",strerror(errno));
		return -1;
	}

	printf("create socket successful\n");
	//将socket_in结构体清空为0
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(8899);
	serv_addr.sin_addr.s_addr = htonl( INADDR_ANY);
	if( bind(listen_fd, (struct sockaddr *)&serv_addr,sizeof(serv_addr))<0){
		printf("bind is failed:[%s]\n",strerror(errno));
		return -1;
	}
	printf("socket:[%d],bind on port:[%d]\n",listen_fd,8899);

	listen(listen_fd,14);
	signal(SIGCHLD, handle_sigchld2);
	while(1){
		client_fd = accept(listen_fd, (struct sockaddr *) &cli_addr, &socklen);
		if(client_fd < 0)       {  
		
  			printf("Accept new client failure: %s\n", strerror(errno));
			continue;               
	       	}
		printf("Accept new client[%s:%d] successfully\n" ,inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
		pid_t		pid;
		int 		status = -1;
		pid = fork();
		if( pid < 0 )
		{
		 printf("fork() create child process failure: %s\n", strerror(errno)); 
		 close(client_fd);
		 continue; 
		}
		else if( pid > 0 )
		{
			/* Parent process close client fd and goes to  accept new socket client again */ 
			waitpid(pid, &status, WNOHANG);
			close(client_fd);
			continue; 
		
		}
		else{
			char                 buf[1024];
			/* Child process close the listen socket fd */
			close(listen_fd);
			printf("Child process start to commuicate with socket client...\n");
			memset(buf, 0, sizeof(buf));
			rv=read(client_fd, buf, sizeof(buf));
			if( rv < 0 ) 
			{
			
				printf("Read data from client sockfd[%d] failure: %s\n", client_fd, strerror(errno)); 
				close(client_fd);
				exit(0);
			}
			printf("Read %d bytes data from Server: %s\n", rv, buf);
			rv = write(client_fd,buf,sizeof(buf));
			if(rv<0){
				printf("write is failed:[%s]\n",strerror(errno));
				close(client_fd);
				exit(0);
			}
			close(client_fd);
			exit(0);
		
		}
		 
		 
	}
	close(listen_fd);
}
