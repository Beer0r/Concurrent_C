#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include "server.h"


/* ---------------- start of main ---------------- */

int main( int argc, char *argv[] ){
	
 //Init
 int newsockfd, client_lenght, pid, tmp_shm_lenght;
 char buffer[MAX_CONTENT_LENGHT+MAX_FILE_LENGHT+sizeof(command)];
 struct sockaddr_in serv_addr, cli_addr;
 int  n, i, x;
 char *myPtr;
 int mykey = 1111;
 void *shared_memory = (void *)0;
 int quit=1;

 //Cleanup on CTRL+C
 signal(SIGINT, cleanup);

 //Create shared memory
 shmid = shmget(mykey, sizeof(struct file_list), 0666 | IPC_CREAT);
 if (shmid == -1) {
  fprintf(stderr, "[%d] - ERROR:  shmget failed.\n",getpid());
  exit(EXIT_FAILURE);
 }
 shared_memory = shmat(shmid, (void *)0, 0);
 if (shared_memory == (void *)-1) {
  fprintf(stderr, "[%d] - ERROR:  shmat failed.\n",getpid());
  exit(EXIT_FAILURE);
 }
 //Put struct in shared memory an memset all entries to '\0'
 shared_list = (struct file_list *)shared_memory;
 for(x = 0; x < NUMBER_OF_FILES; x ++) {
  memset(&shared_list->file_name[x][0], '\0', sizeof(shared_list->file_name[x]));
  memset(&shared_list->file_content[x][0], '\0', sizeof(shared_list->file_content[x]));
 }

 //Create Semaphore
 sem_id_create = semget(IPC_PRIVATE,1,IPC_CREAT | 0660);
 if (sem_id_create == -1) {
  fprintf(stderr, "[%d] - ERROR:  Create Semaphore failed.\n",getpid());
  exit(EXIT_FAILURE);
 }
 if (sem_Init(sem_id_create, 1, 1) == -1) {
  fprintf(stderr, "[%d] - ERROR:  Set Create Semaphore value to 1 failed.\n",getpid()); 
  exit(EXIT_FAILURE);
 } 
 
 //Files Semaphore
 sem_id_files = semget(IPC_PRIVATE,NUMBER_OF_FILES,IPC_CREAT | 0660);
 if (sem_id_files == -1) {
  fprintf(stderr, "[%d] - ERROR:  Files Semaphore.\n",getpid());
  exit(EXIT_FAILURE);
 }
 if (sem_Init(sem_id_files, 1, NUMBER_OF_FILES) == -1) {
  fprintf(stderr, "[%d] - ERROR:  Set Files Semaphore value to 1 failed.\n",getpid()); 
  exit(EXIT_FAILURE);
 } 

 //Call socket
 sockfd = socket(AF_INET, SOCK_STREAM, 0);
 if (sockfd < 0) {
  fprintf(stderr, "[%d] - ERROR:  Opening socket.\n",getpid());
  exit(EXIT_FAILURE);
 }
 
 //Define socket
 bzero((char *) &serv_addr, sizeof(serv_addr));
 serv_addr.sin_family = AF_INET;
 serv_addr.sin_addr.s_addr = INADDR_ANY;
 serv_addr.sin_port = htons(PORT);
 
 //Bind socket
 if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
  fprintf(stderr, "[%d] - ERROR:  Binding socket.\n",getpid());
  exit(EXIT_FAILURE);
 }
 fprintf(stderr, "[%d] - START:  Fileserver is ready.\n",getpid());
 
 //Wait for clients
 listen(sockfd,10);
 client_lenght = sizeof(cli_addr);
 while (1) {
 	
  //Child socket
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &client_lenght);
  if (newsockfd < 0){
   fprintf(stderr, "[%d] - ERROR:  Could not accept.\n",getpid());
   exit(EXIT_FAILURE);
  }
  
  //Create child process
  pid = fork();
  if (pid < 0){
   fprintf(stderr, "[%d] - ERROR:  Fork.\n",getpid());
   exit(EXIT_FAILURE);
  }
  
  //Start child routine
  if (pid == 0){
   close(sockfd);
   child_routine(newsockfd);
   exit(EXIT_SUCCESS);
  }else{
   close(newsockfd);
  }
 }
}

/* ---------------- child routine ---------------- */

void child_routine (int sock){
 //Init
 int n;
 int check_input = 0;
 char buffer[MAX_CONTENT_LENGHT];
 char tmp_buffer[MAX_CONTENT_LENGHT];
 char tmp_content[MAX_CONTENT_LENGHT];
 char buf_list[5];
 char *split;
 char *split_content;
 bzero(buffer,MAX_CONTENT_LENGHT);
 
 //Read socket
 n = read(sock,buffer,MAX_CONTENT_LENGHT+MAX_FILE_LENGHT+sizeof(command));
 if (n < 0){
 	 fprintf(stderr, "[%d] - ERROR:  Reading from socket.\n",getpid());
   exit(EXIT_FAILURE);
 }

 //Only work with copys of buffer
 strcpy(tmp_buffer,buffer);
 strcpy(tmp_content,buffer);
 strncpy(buf_list, buffer, 4);
 buf_list[4] = '\0'; // Add end of String

 //LIST
 if (strcmp(buf_list,"LIST") == 0){
  check_input = 1;
  int check=0;
  int x;
  
  //doList routine
  fprintf(stderr, "[%d] - LIST:   Client wants to list all files.\n",getpid());
  doList(sock);
  fprintf(stderr, "[%d] - LIST:   Sent lsit.\n",getpid());
 }

    split = strtok(tmp_buffer, " ");

 //Create
 if (check_input == 0){
  if (strcmp(split,"CREATE") == 0){
   check_input = 1;
   int x;
   int check = 0;

   //Get file name
   split = strtok(NULL, " ");
   if(split == NULL){
   	sendWrongCommand(sock);
    fprintf(stderr, "[%d] - ERROR:  No file on CREATE command found.\n",getpid());
    exit(EXIT_FAILURE);   	
   }
   char tmp_name[getStringSize(split)];
   strcpy(tmp_name,split);
   
   //Get content lenght
   split = strtok(NULL, " ");
   int tmp_lenght = atoi(split);
   char tmp_create_content[tmp_lenght];
   memset(&tmp_create_content[0], '\0', sizeof(tmp_create_content));

   //Get content
   split_content = strtok(tmp_content, "\n");
   split_content = strtok(NULL, "\n");

   //Adjust content
   for(x = 0; x < tmp_lenght-1; x ++) {
    if(split_content[x] == '\0'){
     check=1;
    }
    if(check==1){
     tmp_create_content[x] = '\0';
    }else{
     tmp_create_content[x] = split_content[x];
    }
   }
   tmp_create_content[tmp_lenght-1] == '\0';

   //doCreate routine
   fprintf(stderr, "[%d] - CREATE: Client wants to create file %s\n",getpid(),tmp_name);
   if(doCreate(tmp_name, tmp_create_content, tmp_lenght, sock)==1){
    fprintf(stderr, "[%d] - CREATE: Created file %s\n",getpid(),tmp_name);
   }
  }
 }

 //READ  
 if (check_input == 0){
  if (strcmp(split,"READ") == 0){
   check_input = 1;
      
   //Split client command
   char tmp_read_name[MAX_FILE_LENGHT];
   split = strtok(NULL, " ");
   strcpy(tmp_read_name,split);
 
   //Delete newline character
   char *newline = strchr( tmp_read_name, '\n' );
   if ( newline )
    *newline = 0;
    
   //doRead routine
   fprintf(stderr, "[%d] - READ:   Client wants to read file %s\n",getpid(),tmp_read_name);
   if(doRead(tmp_read_name, sock)==1){
    fprintf(stderr, "[%d] - READ:   Sent file %s\n",getpid(),tmp_read_name);
   }
  }
 }

 //UPDATE
 if (check_input == 0){
  if (strcmp(split,"UPDATE") == 0){
   check_input = 1;
   int x;
   int check = 0;

   //Get file name
   split = strtok(NULL, " ");
   if(split == NULL){
   	sendWrongCommand(sock);
    fprintf(stderr, "[%d] - ERROR:  No file on UPDATE command found.\n",getpid());
    exit(EXIT_FAILURE);   	
   }
   char tmp_name[getStringSize(split)];
   strcpy(tmp_name,split);
   
   //Get content lenght
   split = strtok(NULL, " ");
   int tmp_lenght = atoi(split);
   char tmp_update_content[tmp_lenght];
   memset(&tmp_update_content[0], '\0', sizeof(tmp_update_content));

   //Get content
   split_content = strtok(tmp_content, "\n");
   split_content = strtok(NULL, "\n");

   //Adjust content
   for(x = 0; x < tmp_lenght-1; x ++) {
    if(split_content[x] == '\0'){
     check=1;
    }
    if(check==1){
     tmp_update_content[x] = '\0';
    }else{
     tmp_update_content[x] = split_content[x];
    }
   }
   tmp_update_content[tmp_lenght-1] == '\0';

   //doUpdate routine
   fprintf(stderr, "[%d] - UPDATE: Client wants to update file %s\n",getpid(),tmp_name);
   if(doUpdate(tmp_name, tmp_update_content, tmp_lenght, sock)==1){
    fprintf(stderr, "[%d] - UPDATE: Updated file %s\n",getpid(),tmp_name);
   }
  }
 }

 //DELETE
 if (check_input == 0){
  if (strcmp(split,"DELETE") == 0){
   check_input = 1;
      
   //Split client command
   char tmp_del_name[MAX_FILE_LENGHT];
   split = strtok(NULL, " ");
   strcpy(tmp_del_name,split);
 
   //Delete newline character
   char *newline = strchr( tmp_del_name, '\n' );
   if ( newline )
    *newline = 0;
    
   //doDelete routine
   fprintf(stderr, "[%d] - DELETE: Client wants to delete file %s\n",getpid(),tmp_del_name);
   if(doDelete(tmp_del_name, sock)==1){
    fprintf(stderr, "[%d] - DELETE: Deleted file %s\n",getpid(),tmp_del_name);
   }
  }
 }

 if (check_input == 0){
  fprintf(stderr, "[%d] - WARNING: No vaild action!\n",getpid());
  sendWrongCommand(sock);
 }
}

/* ---------------- getStringSize ---------------- */

int getStringSize(char * tmp_str){
 int i = 0;
 while(tmp_str[i]!='\0'){
  i++;
 }
 return i;
}

/* ---------------- getFreeLine ---------------- */

int getFreeLine(){
 int x;
 for(x = 0; x < NUMBER_OF_FILES; x ++) {
  if(strcmp(shared_list->file_name[x],"") == 0){
   return x;
  }
 }
 return -1;
}

/* ---------------- getLine ---------------- */

int getLine(char * file_name){
 int x;
 for(x = 0; x < NUMBER_OF_FILES; x ++) {
  if(strcmp(shared_list->file_name[x],file_name) == 0){
   return x;
  }
 }
 return -1;
}

/* ---------------- getSize ---------------- */

int getSize(int size){
 (double)size;
 if(size>=1000000000) return -1;
 if(size/100000000>=1) return 9;
 if(size/10000000>=1) return 8;
 if(size/1000000>=1) return 7;
 if(size/100000>=1) return 6;
 if(size/10000>=1) return 5;
 if(size/1000>=1) return 4;
 if(size/100>=1) return 3;
 if(size/10>=1) return 2;
 if(size>=1) return 1;
 return -1;
}

/* ---------------- doRead ---------------- */

int doRead(char * file_name, int sock){
 int n;

 //Check if File exists
 int index = getLine(file_name);
 if(index>-1){
    
  //Calculate Answer Size & memset arrays
  char tmp_answer[15+getSize(shared_list->content_lenght[index])+shared_list->file_lenght[index]];
  memset(&tmp_answer[0], 0, sizeof(tmp_answer));
  char tmp_content_lenght[getSize(shared_list->content_lenght[index])];
  memset(&tmp_content_lenght[0], 0, sizeof(tmp_content_lenght));
  sprintf(tmp_content_lenght, "%d", shared_list->content_lenght[index]);
  char tmp_answer2[1+shared_list->content_lenght[index]];
  memset(&tmp_answer2[0], 0, sizeof(tmp_answer2));
    
  //Build answer
  strcat(tmp_answer,"FILECONTENT ");
  strcat(tmp_answer,shared_list->file_name[index]);
  strcat(tmp_answer," ");
  strcat(tmp_answer,tmp_content_lenght);
  strcat(tmp_answer,"\n");
  strcat(tmp_answer2,shared_list->file_content[index]);
  strcat(tmp_answer2,"\n");

  //Send answer - header
  n = write(sock,tmp_answer,sizeof(tmp_answer));
  if (n < 0) {
   fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
   exit(EXIT_FAILURE);
  }
  
  //Send answer - content
  n = write(sock,tmp_answer2,sizeof(tmp_answer2));
  if (n < 0) {
   fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
   exit(EXIT_FAILURE);
  }
  return 1;

 //File doesn't exist
 }else{
  n = write(sock,"NOSUCHFILE\n",12);
  if (n < 0) {
   fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
   exit(EXIT_FAILURE);
  }
  return -1;
 }
}

/* ---------------- getFileCount ---------------- */

int getFileCount(){
 int x, y = 0;
  
 //Count files
 for(x = 0; x < NUMBER_OF_FILES; x ++) { 
 	if(shared_list->file_name[x][0] != '\0'){y++;}
 }
 return y;
}

/* ---------------- doList---------------- */

void doList(int sock){
 int x,n;
 int y=getFileCount();
 
 if(y>0){
  //Calculate Answer Size & memset arrays
  char tmp_answer[5+getSize(y)];
  memset(&tmp_answer[0], 0, sizeof(tmp_answer));
  char tmp_count[getSize(y)];
  memset(&tmp_count[0], 0, sizeof(tmp_count));
  sprintf(tmp_count, "%d", y);
  
  //Build answer
  strcat(tmp_answer,"ACK ");
  strcat(tmp_answer,tmp_count);
  strcat(tmp_answer,"\n");
  
  //Send ACK message
  n = write(sock,tmp_answer,sizeof(tmp_answer));
  if (n < 0) {
   fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
   exit(EXIT_FAILURE);
  }
  
  //Send file per file if not empty
  for(x = 0; x < NUMBER_OF_FILES; x ++) { 
   if(shared_list->file_name[x][0] != '\0'){
    n = write(sock,shared_list->file_name[x],shared_list->file_lenght[x]);
    if (n < 0) {
     fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
     exit(EXIT_FAILURE);
    }
    n = write(sock,"\n",1);
    if (n < 0) {
     fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
     exit(EXIT_FAILURE);
    }  
   }
  }

  //No files
 }else{
  n = write(sock,"ACK 0\n",6);
  if (n < 0) {
   fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
   exit(EXIT_FAILURE); 	
  }
 }
}

/* ---------------- doCreate ---------------- */

int doCreate(char * tmp_name, char * tmp_create_content, int tmp_lenght, int sock){
 int free_line,n;

 if(getLine(tmp_name)==-1){

  //Semaphore: Lock Create Process
  sem_P(sem_id_create, 0);
  fprintf(stderr, "[%d] - SEMA:   Accessed critical section - create\n", getpid());
  
  //Get next free line
  free_line = getFreeLine();
  if(free_line == -1){
    n = write(sock,"NOMORESPACE\n",12);
    if (n < 0) {
     fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
     exit(EXIT_FAILURE);
    }
    return -1;
    
  //Create new file  
  }else{  
   strcpy(shared_list->file_name[free_line],tmp_name);
   strcpy(shared_list->file_content[free_line],tmp_create_content);
   shared_list->file_lenght[free_line]=getStringSize(tmp_name);
   shared_list->content_lenght[free_line]=tmp_lenght;
  
   //Semaphore: Unlock Create Process
   sem_V(sem_id_create, 0);
   fprintf(stderr, "[%d] - SEMA:   Exit critical section - create\n", getpid());
   
   //Send answer to client
   n = write(sock,"FILECREATED\n",12);
   if (n < 0) {
    fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
    exit(EXIT_FAILURE);
   }
   return 1;
  }
 }else{
  //Send error messgae to client
  n = write(sock,"FILEEXISTS\n",11);
  if (n < 0) {
   fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
   exit(EXIT_FAILURE);
  }
  return -1;
 } 
}

/* ---------------- doDelete ---------------- */

int doDelete(char * file_name, int sock){
 int n;
 
 //Check if File exists
 int index = getLine(file_name);
 if(index>-1){
    
  //Semaphore: Lock a single File
  sem_P(sem_id_files, index);
  fprintf(stderr, "[%d] - SEMA:   Accessed critical section - file:%i\n", getpid(),index);

  //Memset/Delete file
  memset(&shared_list->file_name[index][0], '\0', sizeof(shared_list->file_name[index]));
  memset(&shared_list->file_content[index][0], '\0', sizeof(shared_list->file_content[index]));
  shared_list->file_lenght[index]=0;
  shared_list->content_lenght[index]=0;

  //Semaphore: Unlock used File
  sem_V(sem_id_files, index);
  fprintf(stderr, "[%d] - SEMA:   Exit critical section - file:%i\n", getpid(),index);
  
  //Send answer - content
  n = write(sock,"DELETED\n",8);
  if (n < 0) {
   fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
   exit(EXIT_FAILURE);
  }
  return 1;

  //File doesn't exist
 }else{
  n = write(sock,"NOSUCHFILE\n",12);
  if (n < 0) {
   fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
   exit(EXIT_FAILURE);
  }
  return -1;
 }
}

/* ---------------- sendWrongCommand ---------------- */

void sendWrongCommand(int sock){
	int n;
	
  //Send back: error on commadn
  n = write(sock,"COMMAND ERROR\n",14);
  if (n < 0) {
   fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
   exit(EXIT_FAILURE);
  }  
}

/* ---------------- doUpdate ---------------- */

int doUpdate(char * tmp_name, char * tmp_update_content, int tmp_lenght, int sock){
 int n, index;

 //Check if File exists
 index = getLine(tmp_name);
 if(index>-1){

  //Semaphore: Lock single file
  sem_P(sem_id_files, index);
  fprintf(stderr, "[%d] - SEMA:   Accessed critical section - file:%i\n", getpid(),index);

  //Memset/Delete file
  memset(&shared_list->file_name[index][0], '\0', sizeof(shared_list->file_name[index]));
  memset(&shared_list->file_content[index][0], '\0', sizeof(shared_list->file_content[index]));
  shared_list->file_lenght[index]=0;
  shared_list->content_lenght[index]=0;
   
  //Update file  
  strcpy(shared_list->file_name[index],tmp_name);
  strcpy(shared_list->file_content[index],tmp_update_content);
  shared_list->file_lenght[index]=getStringSize(tmp_name);
  shared_list->content_lenght[index]=tmp_lenght;

  //Semaphore: Unlock used File
  sem_V(sem_id_files, index);
  fprintf(stderr, "[%d] - SEMA:   Exit critical section - file:%i\n", getpid(),index);
  
  //Send answer to client
  n = write(sock,"UPDATED\n",8);
  if (n < 0) {
   fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
   exit(EXIT_FAILURE);
  }
  return 1;

  //File doesn't exist
 }else{
  n = write(sock,"NOSUCHFILE\n",12);
  if (n < 0) {
   fprintf(stderr, "[%d] - ERROR: Writing socket.\n",getpid());
   exit(EXIT_FAILURE);
  }
  return -1; 
 }
}

/* ---------------- Semaphore P-operation ---------------- */

int sem_P(int sem_id, int count) {
 struct sembuf sem_d;
 sem_d.sem_num = count;
 sem_d.sem_op = -1;
 sem_d.sem_flg = 0;
 if (semop(sem_id,&sem_d,1) == -1) {
  fprintf(stderr, "[%d] - ERROR:  Semaphore P-operation failed.\n",getpid());
  return -1;
 }
 return 0;
}

/* ---------------- Semaphore V-operation ---------------- */

int sem_V(int sem_id, int count) {
 struct sembuf sem_d;
 sem_d.sem_num = count;
 sem_d.sem_op = 1;
 sem_d.sem_flg = 0;
 if (semop(sem_id,&sem_d,1) == -1) {
  fprintf(stderr, "[%d] - ERROR:  Semaphore V-operation failed.\n",getpid());
  return -1;
 }
 return 0;
}

/* ---------------- Init of Semaphore ---------------- */

int sem_Init(int sem_id, int val, int count) {
 union semun arg;
 arg.val = val;
 int x;
 
 for(x = 0; x < count; x ++) {
  if (semctl(sem_id,x,SETVAL,arg) == -1) {
   fprintf(stderr, "[%d] - ERROR:  Semaphore settings on %i.\n",getpid(),x);
   return -1;
  }
 }
 return 0;
}

/* ---------------- cleanup ---------------- */

void cleanup(){
	
 //Delete Semaphores
 semctl(sem_id_create,0,IPC_RMID,0);
 semctl(sem_id_files,NUMBER_OF_FILES,IPC_RMID,0);
 
 //Delete shared memory
 shmctl(shmid,IPC_RMID,NULL);
 
 //Byebye
 fprintf(stderr, "[%d] - CLEAN: Byebye.\n",getpid());
 
 //Close socket
 close(sockfd);
 
 exit(0);
 
}

/* ---------------- end ---------------- */
