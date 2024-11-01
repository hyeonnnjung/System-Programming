/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"

typedef struct {
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];
} pool;

//node == 해당 서버에서 한 보유하고 있는 한 종목
typedef struct node {
    int id;
    int amount;
    int price;
    struct node* left;
    struct node* right;
} Node;


void init_pool(int listenfd, pool *p);
void add_client(int connfd, pool *p);
void check_clients(pool *p);
Node* createNode(int id, int amount, int price);
Node* insertNode(Node* root, int id, int amount, int price);
void deleteTree(Node* root);
Node* searchTree(Node *root, int id);
int parseline(char *buf, char **argv);
void preorder(Node* root, char* result);

Node* root = NULL;


int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    static pool pool;

    if (argc != 2) {
       fprintf(stderr, "usage: %s <port>\n", argv[0]);
       exit(0);
    }

    //stock.txt열어서 메모리 적재 : binary search tree 생성
    FILE* stockTable_open = fopen("stock.txt", "r");
    if(!stockTable_open){
        fprintf(stderr, "error : open stock.txt failed");
        exit(0);
    }
    int id, amount, price;
    while(fscanf(stockTable_open, "%d %d %d", &id, &amount, &price) != -1){
        root = insertNode(root, id, amount, price);
    }

    fclose(stockTable_open);

    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);

    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);

        //만약 새로운 connect 요청이 들어왔다면
        if(FD_ISSET(listenfd, &pool.ready_set)){
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            //서버 측에서 connect 문구 출력
            Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);

            //pool의 clientfd에 새로운 connfd 추가
            add_client(connfd, &pool);
        }

        //connect 요청이 아니라 다른 요청이라면
        check_clients(&pool);
    }
    deleteTree(root);
    exit(0);
}
/* $end echoserverimain */

void init_pool(int listenfd, pool *p){
    int i;
    p->maxi = -1;
    for(i = 0; i < FD_SETSIZE; i++){
        p->clientfd[i] = -1;
    }

    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p){
    int i;
    p->nready--;
    for(i = 0; i < FD_SETSIZE; i++){
        if(p->clientfd[i] < 0){
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);

            FD_SET(connfd, &p->read_set);

            if(connfd > p->maxfd) p->maxfd = connfd;
            if(i > p->maxi) p->maxi = i;
            break;
        }
    }

    if(i == FD_SETSIZE) app_error("add_client error: Too many clients");
}

//순회하면서 출력하는 함수 : show에서 사용
void preorder(Node* root, char* result){
    if(root != NULL){
        char tmp[30];
        sprintf(tmp, "%d %d %d\n", root->id, root->amount, root->price);
        strcat(result, tmp);
        preorder(root->left, result);
        preorder(root->right, result);
    }
}

//tree search
Node* searchTree(Node *root, int id){
    if(root == NULL) return NULL;

    if(id == root->id) return root;
    else if(id > root->id) searchTree(root->right, id);
    else searchTree(root->left, id);
}

int parseline(char *buf, char **argv) {
    char *delim;
    int argc = 0;

    buf[strlen(buf)-1] = ' ';
    while (*buf && (*buf == ' ')) buf++;

    while ((delim = strchr(buf, ' '))) {
       argv[argc++] = buf;
       *delim = '\0';
       buf = delim + 1;
       while (*buf && (*buf == ' ')) buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0) return 1;

    return argc;
}

void check_clients(pool *p){
    int i, connfd, n;
    char buf[MAXLINE];
    rio_t rio;

    for(i = 0; (i <= p->maxi) && (p->nready > 0); i++){
        connfd = p->clientfd[i];
        rio = p->clientrio[i];

        //for문을 돌다가 이벤트가 발생한 fd 발견
        if((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))){
            p->nready--;
            
            //한줄씩 rio에서 읽어서 buf에 저장
            if((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
                //주식 서버 기능 구현

                //서버 측에 몇 바이트 받았는지 출력
                printf("server received %d bytes\n", n);

                char command[MAXLINE];
                strcpy(command, buf);
                char *argv[5];
                int argc = parseline(command, argv);

                //1. show
                if(strcmp(argv[0], "show") == 0){
                    char result[MAXLINE] = {0};
                    preorder(root, result);
                    Rio_writen(connfd, result, MAXLINE);
                }
                else if(strcmp(argv[0], "buy") == 0){
                    int stock_id = atoi(argv[1]);
                    int stock_amount = atoi(argv[2]);
                    //printf("%d\n", stock_amount);

                    //id값이 stock_id인 노드 받아오기
                    Node* target = searchTree(root, stock_id);
                    //printf("%d\n", target->amount);

                    if(stock_amount > target->amount){
                        char result[MAXLINE];
                        sprintf(result, "Not enough left stock\n");
                        Rio_writen(connfd, result, MAXLINE);
                    }
                    else{
                        target->amount -= stock_amount;
                        char result[MAXLINE];
                        sprintf(result, "[buy] success\n");
                        Rio_writen(connfd, result, MAXLINE);

                    }
                }
                else if(strcmp(argv[0], "sell") == 0){
                    int stock_id = atoi(argv[1]);
                    int stock_amount = atoi(argv[2]);

                    //id값이 stock_id인 노드 받아오기
                    Node* target = searchTree(root, stock_id);

                    target->amount += stock_amount;
                    char result[MAXLINE];
                    sprintf(result, "[sell] success\n");
                    Rio_writen(connfd, result, MAXLINE);
                }
            }

            else{
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;

                FILE* stockTable_save = fopen("stock.txt", "w");
                if(!stockTable_save){
                    fprintf(stderr, "error : stock.txt(save) open failed\n");
                    exit(0);
                }

                char result[MAXLINE] = {0};
                preorder(root, result);
                fprintf(stockTable_save, "%s", result);
                fclose(stockTable_save);
            }
        }
    }
}

Node* createNode(int id, int amount, int price){
    Node* node = (Node*)malloc(sizeof(Node));
    node->id = id;
    node->amount = amount;
    node->price = price;
    node->left = NULL;
    node->right = NULL;
    //printf("%d %d %d\n", node->id, node->amount, node->price);
    return node;
}

Node* insertNode(Node* root, int id, int amount, int price){
    if(root == NULL){
        return createNode(id, amount, price);
    }

    if(id > root->id) root->right = insertNode(root->right, id, amount, price);
    else if(id < root->id) root->left = insertNode(root->left, id, amount, price);

    return root;
}

void deleteTree(Node* root){
    if(root == NULL) return;

    deleteTree(root->left);
    deleteTree(root->right);
    free(root);
}