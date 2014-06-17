#ifndef SERVER_H
#define SERVER_H

#define NUMBER_OF_FILES 10
#define MAX_FILE_LENGHT 20
#define MAX_CONTENT_LENGHT 256
#define PORT 8080

struct file_list{
 char file_name[NUMBER_OF_FILES][MAX_FILE_LENGHT];
 char file_content[NUMBER_OF_FILES][MAX_CONTENT_LENGHT];
 int file_lenght[NUMBER_OF_FILES];
 int content_lenght[NUMBER_OF_FILES];
};

union semun {
   int val;                // SETVAL
   struct semid_ds *buf;   // IPC_STAT, IPC_SET
   unsigned short *array;  //GETALL, SETALL
};

struct file_list *shared_list;
int sem_id_create, sem_id_files, shmid, sockfd;
char command[] = "UPDATE  999\n";

//Functions
void child_routine(int sock);
int doRead(char * file_name, int sock);
int doDelete(char * file_name, int sock);
void doList(int sock);
int doCreate(char * tmp_name, char * tmp_create_content, int tmp_lenght, int sock);
int doUpdate(char * tmp_name, char * tmp_update_content, int tmp_lenght, int sock);
void sendWrongCommand(int sock);
int getLine(char * file_name);
int getSize(int size);
int getStringSize(char * tmp_str);
void cleanup();
int sem_P(int sem_id, int count);
int sem_V(int sem_id, int count);
int sem_Init(int sem_id, int val, int count);

#endif /** SERVER_H */
