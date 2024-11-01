#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "hash.h"
#include "bitmap.h"

bool less(const struct list_elem *a, const struct list_elem *b, void *aux) {
    const struct list_item *item_a = list_entry(a, struct list_item, elem);
    const struct list_item *item_b = list_entry(b, struct list_item, elem);
    return item_a->data < item_b->data;
}

unsigned hash_num(const struct hash_elem *e, void *aux){
    int data = e->data;
    return hash_int(data);
}

bool hash_less(const struct hash_elem *a, const struct hash_elem *b, void *aux){
    return a->data < b->data;
}

void square(struct hash_elem *e, void *aux){
    e->data = e->data * e->data;
}

void triple(struct hash_elem *e, void *aux){
    e->data = e->data * e->data * e->data;
}

void destructor(struct hash_elem *e, void *aux){
    free(e);
}

int main(void){
    char command[100];
    char first[50];
    char second[50];
    char third[50];
    char fourth[50];
    char fifth[50];

    struct list **List = malloc(sizeof(struct list*) * 10);
    struct hash **Hash = malloc(sizeof(struct hash*) * 10);
    struct bitmap **Bitmap = malloc(sizeof(struct bitmap*) * 10);

    while(1){
        scanf("%s", command);
        //command == "quit"이면 0을 return
        if(strcmp(command, "quit") == 0) return 0;

        else if(strcmp(command, "list_unique") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];
            list_sort(target, less, NULL);

            char ch = getchar();
            if(ch == ' '){
                scanf("%s", second);
                int index_second = second[strlen(second) - 1] - 48;
                struct list* temp = List[index_second];
                list_unique(target, temp, less, NULL);
            }
            else list_unique(target, NULL, less, NULL);
        }

        else if(strcmp(command, "create") == 0){
            scanf("%s", first);
            scanf("%s", second);
            //입력받은 리스트명의 마지막 숫자를 인덱스로
            int index = second[strlen(second) - 1] - 48;

            //list
            if(strcmp(first, "list") == 0){
                List[index] = (struct list*)malloc(sizeof(struct list));
                list_init(List[index]);
            }
            //hashtable
            else if(strcmp(first, "hashtable") == 0){
                Hash[index] = (struct hash*)malloc(sizeof(struct hash));
                hash_init(Hash[index], hash_num, hash_less, NULL);
            }
            //bitmap
            else if(strcmp(first, "bitmap") == 0){
                scanf("%s", third);
                int size = atoi(third);
                Bitmap[index] = bitmap_create(size);                
            }
        }
        
        else if(strcmp(command, "delete") == 0){
            scanf("%s", first);
            int length = strlen(first);
            char* type = malloc(sizeof(char) * length);
            int index = first[strlen(first) - 1] - 48;
            strncpy(type, first, strlen(first) - 1);

            //list
            if(strcmp(type, "list") == 0){
                free(List[index]);
                List[index] = NULL;
            }
            //hash
            if(strcmp(type, "hash") == 0){
                free(Hash[index]);
                Hash[index] = NULL;
            }
            //bitmap
            if(strcmp(type, "bm") == 0){
                bitmap_destroy(Bitmap[index]);
                Bitmap[index] = NULL;
            }
        }

        else if(strcmp(command, "dumpdata") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            int length = strlen(first);
            char* type = malloc(sizeof(char) * length);
            strncpy(type, first, strlen(first) - 1);
            //dumpdata list
            if(strcmp(type, "list") == 0){
                struct list* target = List[index]; //현재 조작 대상 리스트 포인터
                if(!list_empty(target)){
                    for(struct list_elem* i = list_begin(target); i != list_end(target) ; i = list_next(i)){
                        struct list_item *temp = list_entry(i, struct list_item, elem);
                        printf("%d ", temp->data);
                    }
                    printf("\n");
                }
            }
            //dumpdata hashtable
            else if(strcmp(type, "hash") == 0){
                struct hash* target = Hash[index];
                if(!hash_empty(target)){
                    struct hash_iterator i;
                    hash_first(&i, target);
                    while(hash_next(&i)){
                        printf("%d ", hash_cur(&i)->data);
                    }
                    printf("\n");
                }
            }
            //dumpdata bm
            else if(strcmp(type, "bm") == 0){
                struct bitmap* target = Bitmap[index];
                for(size_t i = 0; i < bitmap_size(target); i++){
                    printf("%d", bitmap_test(target, i));
                }
                printf("\n");
            }
            free(type);
        }

        else if(strcmp(command, "list_push_front") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];
            
            struct list_item* temp = malloc(sizeof(struct list_item));
            scanf("%s", second);
            int data = atoi(second);
            temp->data = data;
            list_push_front(target, &temp->elem);
        }

        else if(strcmp(command, "list_push_back") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            struct list_item* temp = malloc(sizeof(struct list_item));
            scanf("%s", second);
            int data = atoi(second);
            temp->data = data;
            list_push_back(target, &temp->elem);
        }

        else if(strcmp(command, "list_pop_front") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];
            
            list_pop_front(target);
        }

        else if(strcmp(command, "list_pop_back") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];
            
            list_pop_back(target);
        }
        
        else if(strcmp(command, "list_remove") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            scanf("%s", second);
            int first_index = atoi(second);

            struct list_elem* i = list_begin(target);
            while(first_index > 0){
                i = list_next(i);
                first_index--;
            }

            list_remove(i);
        }

        else if(strcmp(command, "list_reverse") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            list_reverse(target);
        }

        else if(strcmp(command, "list_shuffle") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            list_shuffle(target);
        }

        else if(strcmp(command, "list_sort") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            list_sort(target, less, NULL);
        }

        else if(strcmp(command, "list_splice") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            scanf("%s", second);
            int destination_index = atoi(second);
            int destination_index2 = atoi(second);

            scanf("%s", third);
            int index2 = third[strlen(third) - 1] - 48;
            struct list* target2 = List[index2];

            scanf("%s", fourth);
            int start_index = atoi(fourth);
            int start_index2 = atoi(fourth);
            
            scanf("%s", fifth);
            int finish_index = atoi(fifth);
            int finish_index2 = atoi(fifth);

            list_sort(target, less, NULL);
            list_sort(target2, less, NULL);

            struct list_elem* before_element = list_begin(target);
            while(destination_index2 > 0){
                before_element = list_next(before_element);
                destination_index2--;
            }

            struct list_elem* start_element = list_begin(target2);
            while(start_index2 > 0){
                start_element = list_next(start_element);
                start_index2--;
            }

            struct list_elem* finish_element = list_begin(target2);
            while(finish_index2 > 0){
                finish_element = list_next(finish_element);
                finish_index2--;
            }

            list_splice(before_element, start_element, finish_element);
        }

        else if(strcmp(command, "list_swap") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            scanf("%s", second);
            int first_index = atoi(second);
            int first_index2 = atoi(second);
            scanf("%s", third);
            int second_index = atoi(third);
            int second_index2 = atoi(third);

            struct list_elem* first_element = list_begin(target);
            while(first_index > 0){
                first_element = list_next(first_element);
                first_index--;
            }

            struct list_elem* second_element = list_begin(target);
            while(second_index > 0){
                second_element = list_next(second_element);
                second_index--;
            }

            list_swap(first_element, second_element);

            int last_index = (int)list_size(target)-1;
            if(first_index2 == 0) target->head.next = second_element;
            if(second_index2 == last_index) target->tail.prev = first_element;
        }

        else if(strcmp(command, "list_front") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            struct list_elem* elem_temp = list_front(target);
            struct list_item* item_temp = list_entry(elem_temp, struct list_item, elem);
            printf("%d\n", item_temp->data);
        }

        else if(strcmp(command, "list_back") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            struct list_elem* elem_temp = list_back(target);
            struct list_item* item_temp = list_entry(elem_temp, struct list_item, elem);
            printf("%d\n", item_temp->data);
        }

        else if(strcmp(command, "list_insert") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            scanf("%s", second);
            int before_index = atoi(second);
            int before_index2 = atoi(second);
            scanf("%s", third);
            int data = atoi(third);

            struct list_item* temp = malloc(sizeof(struct list_item));
            temp->data = data;

            struct list_elem* before_element = list_begin(target);
            while(before_index > 0){
                before_element = list_next(before_element);
                before_index--;
            }

            list_insert(before_element, &temp->elem);
        }

        else if(strcmp(command, "list_empty") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            if(list_empty(target) == 1) printf("true\n");
            else printf("false\n");
        }

        else if(strcmp(command, "list_size") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            printf("%zu\n", list_size(target));
        }

        else if(strcmp(command, "list_max") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            struct list_elem *temp_element = list_max(target, less, NULL);
            const struct list_item *temp_item = list_entry(temp_element, struct list_item, elem);
            printf("%d\n", temp_item->data);
        }

        else if(strcmp(command, "list_min") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            struct list_elem *temp_element = list_min(target, less, NULL);
            const struct list_item *temp_item = list_entry(temp_element, struct list_item, elem);
            printf("%d\n", temp_item->data);
        }

        else if(strcmp(command, "list_insert_ordered") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct list* target = List[index];

            scanf("%s", second);
            int data = atoi(second);

            struct list_item* temp = malloc(sizeof(struct list_item));
            temp->data = data;

            list_insert_ordered(target, &temp->elem, less, NULL);
        }

        else if(strcmp(command, "hash_insert") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct hash* target = Hash[index];

            scanf("%s", second);
            int data = atoi(second);

            struct hash_elem* temp = malloc(sizeof(struct hash_elem));
            temp->data = data;

            hash_insert(target, temp);
        }

        else if(strcmp(command, "hash_delete") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct hash* target = Hash[index];

            scanf("%s", second);
            int data = atoi(second);

            struct hash_elem* temp = malloc(sizeof(struct hash_elem));
            temp->data = data;

            hash_delete(target, temp);
        }

        else if(strcmp(command, "hash_empty") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct hash* target = Hash[index];

            if(hash_empty(target) == 1) printf("true\n");
            else printf("false\n");
        }

        else if(strcmp(command, "hash_size") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct hash* target = Hash[index];
            
            printf("%zu\n", hash_size(target));
        }

        else if(strcmp(command, "hash_clear") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct hash* target = Hash[index];

            hash_clear(target, destructor);
        }

        else if(strcmp(command, "hash_apply") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct hash* target = Hash[index];

            scanf("%s", second);
            if(strcmp(second, "square") == 0) hash_apply(target, square);
            else if(strcmp(second, "triple") == 0) hash_apply(target, triple);
        }

        else if(strcmp(command, "hash_find") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct hash* target = Hash[index];

            scanf("%s", second);
            int data = atoi(second);

            struct hash_elem* temp = malloc(sizeof(struct hash_elem));
            temp->data = data;

            struct hash_elem* temp2 = hash_find(target, temp);
            if(temp2 != NULL) printf("%d\n", temp2->data);
        }

        else if(strcmp(command, "hash_replace") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct hash* target = Hash[index];

            scanf("%s", second);
            int data = atoi(second);

            struct hash_elem* temp = malloc(sizeof(struct hash_elem));
            temp->data = data;

            hash_replace(target, temp);
        }

        else if(strcmp(command, "bitmap_mark") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            int bit_idx = atoi(second);

            bitmap_mark(target, (size_t)bit_idx);
        }

        else if(strcmp(command, "bitmap_all") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            int start_idx = atoi(second);

            scanf("%s", third);
            int cnt = atoi(third);

            if(bitmap_all(target, (size_t)start_idx, (size_t)cnt)) printf("true\n");
            else printf("false\n");
        }

        else if(strcmp(command, "bitmap_any") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            int start_idx = atoi(second);

            scanf("%s", third);
            int cnt = atoi(third);

            if(bitmap_any(target, (size_t)start_idx, (size_t)cnt)) printf("true\n");
            else printf("false\n");
        }

        else if(strcmp(command, "bitmap_contains") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            int start_idx = atoi(second);

            scanf("%s", third);
            int cnt = atoi(third);

            scanf("%s", fourth);
            bool data;
            if(strcmp(fourth, "true") == 0) data = 1;
            else data = 0;

            if(bitmap_contains(target, (size_t)start_idx, (size_t)cnt, data)) printf("true\n");
            else printf("false\n");
        }

        else if(strcmp(command, "bitmap_count") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            int start_idx = atoi(second);

            scanf("%s", third);
            int cnt = atoi(third);

            scanf("%s", third);
            bool data;
            if(strcmp(third, "true") == 0) data = 1;
            else data = 0;

            printf("%zu\n", bitmap_count(target, (size_t)start_idx, (size_t)cnt, data));
        }

        else if(strcmp(command, "bitmap_dump") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            bitmap_dump(target);
        }

        else if(strcmp(command, "bitmap_flip") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            int bit_idx = atoi(second);

            bitmap_flip(target, (size_t)bit_idx);
        }

        else if(strcmp(command, "bitmap_none") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            int start_idx = atoi(second);

            scanf("%s", third);
            int cnt = atoi(third);

            if(bitmap_none(target, (size_t)start_idx, (size_t)cnt)) printf("true\n");
            else printf("false\n");
        }

        else if(strcmp(command, "bitmap_reset") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            int bit_idx = atoi(second);

            bitmap_reset(target, (size_t)bit_idx);
        }

        else if(strcmp(command, "bitmap_scan_and_flip") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            int start_idx = atoi(second);

            scanf("%s", third);
            int cnt = atoi(third);

            scanf("%s", fourth);
            bool data;
            if(strcmp(fourth, "true") == 0) data = 1;
            else data = 0;

            printf("%zu\n", bitmap_scan_and_flip(target, (size_t)start_idx, (size_t)cnt, data));
        }

        else if(strcmp(command, "bitmap_scan") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            int start_idx = atoi(second);

            scanf("%s", third);
            int cnt = atoi(third);

            scanf("%s", fourth);
            bool data;
            if(strcmp(fourth, "true") == 0) data = 1;
            else data = 0;

            printf("%zu\n", bitmap_scan(target, (size_t)start_idx, (size_t)cnt, data));
        }

        else if(strcmp(command, "bitmap_set_all") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            bool data;
            if(strcmp(second, "true") == 0) data = 1;
            else data = 0;

            bitmap_set_all(target, data);
        }

        else if(strcmp(command, "bitmap_set_multiple") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            int start_idx = atoi(second);

            scanf("%s", third);
            int cnt = atoi(third);

            scanf("%s", fourth);
            bool data;
            if(strcmp(fourth, "true") == 0) data = 1;
            else data = 0;

            bitmap_set_multiple(target, (size_t)start_idx, (size_t)cnt, data);
        }

        else if(strcmp(command, "bitmap_set") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            int start_idx = atoi(second);

            scanf("%s", third);
            bool data;
            if(strcmp(third, "true") == 0) data = 1;
            else data = 0;

            bitmap_set(target, (size_t)start_idx, data);
        }

        else if(strcmp(command, "bitmap_size") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            printf("%zu\n", bitmap_size(target));
        }

        else if(strcmp(command, "bitmap_test") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            size_t idx = (size_t)atoi(second);

            if(bitmap_test(target, idx)) printf("true\n");
            else printf("false\n");
        }

        else if(strcmp(command, "bitmap_expand") == 0){
            scanf("%s", first);
            int index = first[strlen(first) - 1] - 48;
            struct bitmap* target = Bitmap[index];

            scanf("%s", second);
            int cnt = atoi(second);

            bitmap_expand(target, cnt);
        }
    }
}