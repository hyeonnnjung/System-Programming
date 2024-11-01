/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#define NTHREADS 1024
#define SBUFSIZE 1024

//node == 해당 서버에서 한 보유하고 있는 한 종목
typedef struct node {
    int id;
    int amount;
    int price;
    struct node* left;
    struct node* right;
    int readcnt;
    sem_t mutex;
    sem_t w;
} Node;

typedef struct {
    int *buf;
    int n;
    int front; //buf[(front+1)%n] = first item
    int rear; //buf[rear%n] = last item
    sem_t mutex;
    sem_t slots; //비어있는 slot의 개수
    sem_t items; //buf에서 채워져있는 slot의 개수
} sbuf_t;


Node* createNode(int id, int amount, int price);
Node* insertNode(Node* root, int id, int amount, int price);
void deleteTree(Node* root);
Node* searchTree(Node *root, int id);
int parseline(char *buf, char **argv);
void preorder(Node* root, char* result);
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);
void *thread(void *vargp);

Node* root = NULL;
sbuf_t sbuf;
sem_t mutex;


int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid;

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

    sbuf_init(&sbuf, SBUFSIZE);
    Sem_init(&mutex, 0, 1);

    for(int i = 0; i < NTHREADS; i++){
        Pthread_create(&tid, NULL, thread, NULL);
    }

    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        sbuf_insert(&sbuf, connfd);
    }
    
    deleteTree(root);
    sbuf_deinit(&sbuf);
    exit(0);
}
/* $end echoserverimain */


//순회하면서 출력하는 함수 : show에서 사용
void preorder(Node* root, char* result){
    if(root != NULL){
        P(&(root->mutex));
        (root->readcnt)++;
        if(root->readcnt == 1){
            P(&(root->w));
        }
        V(&(root->mutex));

        char tmp[30];
        sprintf(tmp, "%d %d %d\n", root->id, root->amount, root->price);
        strcat(result, tmp);

        P(&(root->mutex));
        (root->readcnt)--;
        if(root->readcnt == 0){
            V(&(root->w));
        }
        V(&(root->mutex));

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

Node* createNode(int id, int amount, int price){
    Node* node = (Node*)malloc(sizeof(Node));
    node->id = id;
    node->amount = amount;
    node->price = price;
    node->left = NULL;
    node->right = NULL;
    node->readcnt = 0;
    Sem_init(&(node->mutex), 0, 1);
    Sem_init(&(node->w), 0, 1);
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

void sbuf_init(sbuf_t *sp, int n){
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = 0;
    sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);   
}

void sbuf_deinit(sbuf_t *sp){
    Free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, int item){
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear) % (sp->n)] = item;
    V(&sp->mutex);
    V(&sp->items);
}

//buf의 첫번째 item 제거하는 함수
int sbuf_remove(sbuf_t *sp){
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front) % (sp->n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}

void *thread(void *vargp){
    Pthread_detach(pthread_self());

    while(1){
        int n;
        char buf[MAXLINE];
        rio_t rio;

        int connfd = sbuf_remove(&sbuf);
        Rio_readinitb(&rio, connfd);

        //주식 서버 기능
        //해당 클라이언트가 buy, sell, show 명령어 수행하고 txt 파일을 update 할 동안 lock
        while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
            P(&mutex);
            //서버 측에 몇 바이트 받았는지 출력
            printf("server received %d bytes\n", n);
            //printf("server received %d bytes\n", n);

            char command[MAXLINE];
            //printf("server received %d bytes : %s\n", n, command);
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
                    P(&(target->w));
                    target->amount -= stock_amount;
                    char result[MAXLINE];
                    sprintf(result, "[buy] success\n");
                    Rio_writen(connfd, result, MAXLINE);
                    V(&(target->w));
                }
            }
            else if(strcmp(argv[0], "sell") == 0){
                int stock_id = atoi(argv[1]);
                int stock_amount = atoi(argv[2]);

                //id값이 stock_id인 노드 받아오기
                Node* target = searchTree(root, stock_id);

                P(&(target->w));  
                target->amount += stock_amount;
                char result[MAXLINE];
                sprintf(result, "[sell] success\n");
                Rio_writen(connfd, result, MAXLINE);
                V(&(target->w));
            }
            V(&mutex);
        }
        Close(connfd);

        P(&mutex);
        FILE* stockTable_save = fopen("stock.txt", "w");
        if(!stockTable_save){
            fprintf(stderr, "error : stock.txt(save) open failed\n");
            exit(0);
        }

        char result[MAXLINE] = {0};
        preorder(root, result);
        fprintf(stockTable_save, "%s", result);
        fclose(stockTable_save);

        V(&mutex);
    }
}

//내가 파일 업데이트 할 동안 다른 스레드 업데이트 하지마!
//내가 이 노드 읽을 동안 다른 스레드에서 readcnt(전역변수) 업데이트 하지마!
//이 노드를 읽는 스레드가 하나라도 있으면 이 노드값 업데이트 하지마!