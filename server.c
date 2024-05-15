#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

//#define PORT     (in_port_t)50000 /* サーバ(自分)のポート番号 */
#define BUF_LEN 512 /* 送受信のバッファの大きさ */
#define NumOfItems 3
#define NAME_LEN 8 /* 送受信のバッファの大きさ */

void sold_funk(items, sa_items, soc);
void *ThreadMain(void *threadArg);
static pthread_mutex_t MyMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct Item
{
	char name[NAME_LEN];
	int price;
	int stocks;
} ITEM;

/*販売記録用*/
typedef struct Sales_record
{
	char name[NAME_LEN];
	int num;
	int total_price;
} sales;

struct ThreadArgs
{
	int soc;
	ITEM *p;
	sales *l;
};

int main(int argc, char *argv[])
{
	/* 変数宣言 */
	struct sockaddr_in me; /* サーバ(自分)の情報   */
	int soc_waiting;	   /* listenするソケット   */
	int soc;			   /* 送受信に使うソケット */
	char buf[BUF_LEN];	   /* 送受信のバッファ     */
	pid_t result_pid;
	int n; /* 読み込まれたバイト数  */
	pthread_t threadID;
	struct ThreadArgs *threadArgs;
	sales *sales_record;

	if (argc != 2)
		exit(-1);

	ITEM items[NumOfItems] = {
		{"coke", 100, 2},
		{"water", 120, 10},
		{"cola", 130, 5}};

	sales sa_items[NumOfItems] = {
		{"coke", 0, 0},
		{"water", 0, 0},
		{"cola", 0, 0}};

	/* サーバ(自分)のアドレスを sockaddr_in 構造体に格納  */
	memset((char *)&me, 0, sizeof(me));
	me.sin_family = AF_INET;
	me.sin_addr.s_addr = htonl(INADDR_ANY);
	me.sin_port = htons(atoi(argv[1]));

	/* IPv4でストリーム型のソケットを作成  */
	if ((soc_waiting = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		exit(1);
	}

	/* サーバ(自分)のアドレスをソケットに設定 */
	if (bind(soc_waiting, (struct sockaddr *)&me, sizeof(me)) == -1)
	{
		perror("bind");
		exit(1);
	}

	/* ソケットで待ち受けることを設定 */
	listen(soc_waiting, 2);

	while (1)
	{
		/* 接続要求が来るまでブロックする */
		soc = accept(soc_waiting, NULL, NULL);
		/* 引数用にメモリを新しく確保  */
		if ((threadArgs = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs))) == NULL)
			fprintf(stderr, "malloc failed\n"), exit(1);
		threadArgs->soc = soc;
		threadArgs->p = items;
		threadArgs->l = sa_items;
		/* スレッドを生成  */
		if (pthread_create(&threadID, NULL, ThreadMain, (void *)threadArgs) != 0)
			fprintf(stderr, "pthread_create() failed\n"), exit(1);
	}
}

//スレッド関数
void *ThreadMain(void *threadArgs)
{
	int soc, num, n;
	char buf[BUF_LEN];
	ITEM *p;
	sales *l;
	/* 戻り時にスレッドのリソース割当を解除 */
	pthread_detach(pthread_self());
	/* 引数からの値を取り出す */
	soc = ((struct ThreadArgs *)threadArgs)->soc;
	p = ((struct ThreadArgs *)threadArgs)->p;
	l = ((struct ThreadArgs *)threadArgs)->l;
	free(threadArgs);
	//自販機の関数を呼ぶ
	sold_funk(p, l, soc);
	close(soc);
}

//自販機の関数
void sold_funk(ITEM *items, sales *sa_items, int soc)
{
	char buf[BUF_LEN];
	int n, i, money = 0, total_money = 0;
	char money_text[10];
	char text[50], text2[50];
	char comp_buf[BUF_LEN];

	sprintf(buf, "soc=%d\n", soc);
	write(1, buf, strlen(buf));

	for (i = 0; i < NumOfItems; i++)
	{
		memset((char *)&buf, 0, sizeof(buf));
		if (items[i].stocks <= 0)
		{
			sprintf(buf, "Item name: %s  sold out!\n", items[i].name);
		}
		else
		{
			sprintf(buf, "Item name: %s  price: %d stocks: %d\n", items[i].name, items[i].price, items[i].stocks);
		}
		write(soc, buf, sizeof(buf));
	}
	/*この文字列で、クライアントは商品受け取るループから抜ける*/

	write(soc, "Please choose a juice\n", 22);

	do
	{

		/*クライアントが選んだ商品を読み込む*/
		n = read(soc, buf, BUF_LEN);

		// write(1, buf, n);

		/*comp_bufにbufを保存して、比較対象にする*/
		memset((char *)&comp_buf, 0, sizeof(comp_buf));
		strcpy(comp_buf, buf);
		memset((char *)&buf, 0, sizeof(buf));

		/*ロックする*/
		pthread_mutex_lock(&MyMutex);

		/*クライアントから聞かれた商品があるか判別*/
		for (i = 0; i < NumOfItems; i++)
		{
			/*客が選んだ商品が商品リストにあるか*/
			if (strncmp(comp_buf, items[i].name, strlen(items[i].name)) == 0)
			{ /*ストックがあるか*/
				if (items[i].stocks <= 0)
				{
					memset((char *)&buf, 0, sizeof(buf));
					strcpy(buf, "nothing\n");
					break;
				}
				memset((char *)&buf, 0, sizeof(buf));
				strcpy(buf, "OK\n");
				break;
			}
			else
			{
				memset((char *)&buf, 0, sizeof(buf));
				strcpy(buf, "NG\n");
			}
		}

		/*お金を入れてもらうか選びなおすかを送信*/
		/*商品があるとき*/
		if (strncmp(buf, "OK", 2) == 0)
		{
			memset((char *)&buf, 0, sizeof(buf));
			strcpy(buf, "please insert your money:");
			write(soc, buf, strlen(buf));
			// write(1, buf, strlen(buf));
		}
		/*在庫切れの時*/
		else if (strncmp(buf, "nothing", 7) == 0)
		{
			memset((char *)&buf, 0, sizeof(buf));
			strcpy(buf, "Sold out!\n");
			write(soc, buf, strlen(buf));
			// write(1, buf, strlen(buf));
			pthread_mutex_unlock(&MyMutex);
			continue;
		}
		else if (strncmp(buf, "NG", 2) == 0)
		{
			memset((char *)&buf, 0, sizeof(buf));
			strcpy(buf, "Please choose a juice again!\n");
			write(soc, buf, strlen(buf));
			// write(1, buf, strlen(buf));
			pthread_mutex_unlock(&MyMutex);
			continue;
		}

		/*お金を受け取る*/
		memset((char *)&buf, 0, sizeof(buf));
		n = read(soc, buf, BUF_LEN);

		// write(1, buf, n);

		/*送られた数字の文字列を数字に変換*/
		money = atoi(buf);

		memset((char *)&buf, 0, sizeof(buf));

		/*商品の値段ともらったお金の比較*/
		do
		{
			memset((char *)&buf, 0, sizeof(buf));
			/*商品の値段と等しい*/
			if (items[i].price == money)
			{
				/*商品の在庫を減らす*/
				items[i].stocks = items[i].stocks - 1;
				/*販売数を増やす*/
				sa_items[i].num = sa_items[i].num + 1;
				/*合計金額を増やしていく*/
				sa_items[i].total_price = sa_items[i].total_price + items[i].price;
				strcpy(buf, "Thank you!\n");
				write(soc, buf, strlen(buf));
				// write(1, buf, strlen(buf));
				break;
			}
			/*入れてもらったお金が商品より大きい*/
			else if (money > items[i].price)
			{
				/*お釣りの計算*/
				money = money - items[i].price;
				/*在庫を減らす*/
				items[i].stocks = items[i].stocks - 1;
				/*販売数を増やす*/
				sa_items[i].num = sa_items[i].num + 1;
				/*合計金額を増やしていく*/
				sa_items[i].total_price = sa_items[i].total_price + items[i].price;
				/*お金を文字列に変換*/
				sprintf(money_text, "%d\n", money);
				strcpy(text, "Thank you!\nThis is your change:");
				/*文字を結合*/
				strcat(text, money_text);
				strcpy(buf, text);
				write(soc, buf, strlen(buf));
				// write(1, buf, strlen(buf));
				break;
			}
			else if (money < items[i].price)
			{
				sprintf(money_text, "%d\n", money);
				strcpy(text, "now:");
				strcpy(text2, "please insert your money more:");
				strcat(text, money_text);
				strcat(text, text2);
				strcpy(buf, text);
				write(soc, buf, strlen(buf));
				// write(1, buf, strlen(buf));
				n = read(soc, buf, BUF_LEN);
				money = money + atoi(buf);
			}
		} while (1);

		pthread_mutex_unlock(&MyMutex);

		/*売上金額や販売数を表示*/
		for (i = 0; i < NumOfItems; i++)
		{
			memset((char *)&buf, 0, sizeof(buf));

			sprintf(buf, "Item name: %s  num: %d price: %d\n",
					sa_items[i].name, sa_items[i].num, sa_items[i].total_price);
			total_money = total_money + sa_items[i].total_price;
			write(1, buf, sizeof(buf));
		}
		memset((char *)&buf, 0, sizeof(buf));
		sprintf(buf, "total_money: %d\n", total_money);
		write(1, buf, sizeof(buf));

		break;

	} while (1);
}
