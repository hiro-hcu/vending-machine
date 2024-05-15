#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>


//#define PORT     (in_port_t)50000 /* サーバ(相手)のポート番号 */
#define BUF_LEN  512              /* 送受信のバッファの大きさ */


int main(int argc, char* argv[])
{
	struct hostent  *server_ent;
	struct sockaddr_in  server;
	int    soc;  /* descriptor for socket */
	int n=0;
	//char   ip_str[]="127.0.0.1";
	struct in_addr  ip;
	char   buf[BUF_LEN];
	char   comp_buf[BUF_LEN];

	if(argc !=3)
	  exit(-1);
	
	inet_aton(argv[1], &ip);

	memset((char *)&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[2]));
	memcpy((char *)&server.sin_addr, &ip, sizeof(ip));

	/* IPv4でストリーム型のソケットを作成  */
	if((soc = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket");
		exit(1);
	}

	/* サーバ(相手)に接続 サーバに接続を要求、 */
	if(connect(soc, (struct sockaddr *)&server, sizeof(server)) == -1){
		perror("connect");
		exit(1);
	}

	/*商品リストを受け取る*/
	while (1)
		{
			memset((char *)&buf, 0, sizeof(buf));
			n = read(soc, buf, BUF_LEN);
			write(1, buf, n);
			if (strstr(buf, "Please choose a juice\n") != NULL)
			{
				break;
			}
		}

	/* 通信のループ */
	do
	{
		int n; /* 読み込まれたバイト数  */

		// n = read(0, buf, BUF_LEN); //標準入力0を読む　キーボード入力0 エンターでbufに記入
		// n = read(soc, buf, BUF_LEN); /* ソケットsocから読む   */
		// write(1, buf, n);            /* 標準出力1に書き出す   */
		// write(soc, buf, n);          /* ソケットsocに書き出す */
		
		memset((char *)&buf, 0, sizeof(buf));
		n = read(0, buf, BUF_LEN);
		write(soc, buf, n);

		memset((char *)&buf, 0, sizeof(buf));
		n = read(soc, buf, BUF_LEN); /* ソケットsocから読む   */
		write(1, buf, strlen(buf));

	} while (strncmp(buf, "Thank you!", 10) != 0); /* 終了判定 */

	/* ソケットを閉じる */
	close(soc);
}
