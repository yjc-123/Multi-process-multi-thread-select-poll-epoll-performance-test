#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
 #include <arpa/inet.h>
#include <sys/socket.h>
       #include <netinet/in.h>
       #include <arpa/inet.h>
#define MSG "hello"
int main(){
	int 				cli_fd;
	struct sockaddr_in		cli_addr;
	socklen_t 			addrlen;
	int				rv;
	char				buf[1024];	
	cli_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	memset(&cli_addr,0,sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port = htons(8899);
	inet_aton("59.110.42.24", &cli_addr.sin_addr);
	if(cli_fd < 0){
		printf("creat socket is faild[%s]\n",strerror(errno));
		printf("F");
		return -1;
	
	}
	//printf("create socket is successful:[%d]\n",cli_fd);
	
	if(connect(cli_fd, (struct sockaddr *)&cli_addr,sizeof(cli_addr))<0){
		printf("connect is faild[%s]\n",strerror(errno));
		printf("F");
		return 0;
	}
	//printf("connect is successful\n");
		/*
		rv = read(cli_fd, buf, sizeof(buf));
		if(rv <0){
			printf("read is faild[%s]\n",strerror(errno));
			continue;
		}
		printf("data is :[%s]",buf);
		*/
		
		
		if(write(cli_fd, &MSG, sizeof(MSG))<0){
			printf("write is faild[%s]\n",strerror(errno));
			close(cli_fd);
	//		printf("失败");
			exit(-1);
		}
	//	printf("write is successful,\n");
		memset(buf,0,sizeof(buf));
		rv = read(cli_fd, buf, sizeof(buf));
		if(rv<0){
			printf("read is failed:[%s]\n",strerror(errno));
			close(cli_fd);
	//	printf("失败");
			exit(-1);	
		}
	//	printf("read is successful,data is [%s]\n",buf);
		printf("OK");
		close(cli_fd);
		exit(0);
	
	
}
