#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <poll.h> // poll
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>   //random
#include <stdbool.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>

struct messg { 
     long mtype;
     char mtext [ 1024 ];
};

void handler(int signumber){
  printf("***Signal %i has arrived for %i!***\n", signumber, getpid());
}

void writeTopipe(int *pipefd){
	char tru[7] = "middle";
	char tru2[7] = "small ";
	char tru3[7] = " big  ";
	close(pipefd[0]); 			
	write(pipefd[1],tru3,7); //writes to the pipe
    close(pipefd[1]);  
	fflush(NULL); 
	printf("Child write to pipe : %s \n ",tru);
}

void readFrompipe(int *pipefd, char*truck){
	char tru[7];
	close(pipefd[1]);  

    read(pipefd[0],&tru,7);
	printf("Parent read from pipe : %s \n",tru);

strcpy(truck,tru);
	close(pipefd[0]);
}

int initMsg(char* argv[]){
    key_t key = ftok(argv[0],1); 
    int msgID = msgget( key, 0600 | IPC_CREAT ); 
    if (msgID < 0) {  perror("msgget error");  exit(EXIT_FAILURE); }
	
	return msgID;
}

void sendMsg(int msgID,char* arr, int *amount1){
	
	char truc[7];
	int amount = 15000;
	strcpy(truc, arr);
	//printf("arr: %s  \n", truc);
	if(strcmp(truc,"middle")==0){
		amount = 15000;
	}
	if(strcmp(truc,"small ")==0){
		amount = 10000;
	}
	if(strcmp(truc," big  ")==0){
		amount = 20000;
	}
	
	*amount1 = amount;
	struct messg m;
	m.mtype = 5;
	char  buf[1024];	
    snprintf(buf, sizeof(buf), "%d", amount);
	
	strcpy(m.mtext, buf);
	int status = msgsnd(msgID,&m, (strlen ( m.mtext ) + 1), 0 );
		
	printf("Parent write to message queue : %s  \n", m.mtext);
		
	if ( status < 0 ) {  perror("msgsnd error");  exit(EXIT_FAILURE); }
			
}

void readMsg(int msgID){
    int status,i;
	struct messg m; 	
 
    status = msgrcv(msgID, &m, 1024, 5, 0 ); 
    if ( status < 0 ) {  perror("msgrcv error");  exit(EXIT_FAILURE); }
	
	printf("Child %i read from message queue (arriving amount): %s  \n",getpid(), m.mtext);	
 

 //destroy msg queue
 //if ( msgctl( msgID, IPC_RMID, NULL ) < 0 ) 
	// {  perror("msgctl error");  exit(EXIT_FAILURE); }
 
}

int initShm(char* argv[]){
	key_t key;
    int sh_mem_id;
    char *s;
	// key creation
    key=ftok(argv[0],1);
    // create shared memory for reading and writing (500 bytes )
    sh_mem_id=shmget(key,500,IPC_CREAT|S_IRUSR|S_IWUSR);
    // to connect the shared memory, 
  
	return sh_mem_id;
}


void writeToShm(int amount,char* s){
	printf("Parent write to shared memory: \n ");
	int money = amount;
	bool notEnough = rand() % 100 < 10;
	if(notEnough){
		printf("size of ordered truck not enough, price twice the origin :) \n ");
		money = 2*money;
	}
	char buffer[1024];                   
    char  buf[1024];	
    snprintf(buf, sizeof(buf), "%d", money);
	strcpy(buffer, buf);
	
    strcpy(s,buffer);
    printf("Parent write to shared memory : %s \n",buffer);
	
    shmdt(s);	   
	
}

void readFromShm(char* s){
	 printf("Child %i  read form shared memory : %s \n",getpid(),s);
	// it releases the shared memory 
    shmdt(s);
}


int semaphore_create(const char* pathname,int semaphore_value){
    int semid;
    key_t key;
    
    key=ftok(pathname,1);    
    if((semid=semget(key,1,IPC_CREAT|S_IRUSR|S_IWUSR ))<0)
	perror("semget");
    // semget 2. parameter is the number of semaphores   
    if(semctl(semid,0,SETVAL,semaphore_value)<0)    //0= first semaphores
        perror("semctl");
       
    return semid;
}

void semaphore_operation(int semid, int op){

    struct sembuf operation;
    
    operation.sem_num = 0;
    operation.sem_op  = op; // op=1 up, op=-1 down 
    operation.sem_flg = 0;
    
    if(semop(semid,&operation,1)<0) // 1 number of sem. operations
        perror("semop");	    
}

void semaphore_delete(int semid){
      semctl(semid,0,IPC_RMID);
}


int main(int argc,char ** argv){

    
  signal(SIGTERM,handler); //handler = SIG_IGN - ignore the signal (not SIGKILL,SIGSTOP), 
                           //handler = SIG_DFL - back to default behavior 
  
  //1. unamed pipe:
  int pipefd[2]; 
  if (pipe(pipefd) == -1) {perror("Opening error!"); exit(EXIT_FAILURE);}
  
  //2. create Messagequeue:
  int msgID = initMsg(argv);
  
  
  //3. init shared memorry:
  int sh_mem_id= initShm(argv);
  char * sm  = shmat(sh_mem_id,NULL,0);
  
  //4. create semaphore:
  int semID = semaphore_create(argv[0],0); 
  
  
  pid_t child=fork();
  if (child<0){perror("The fork calling was not succesful\n"); exit(1);}
   
  if (child>0) //parent
  { 
    pause(); //waits till a signal arrive 
    printf("Signal arrived\n",SIGTERM);
	sleep(1);
	char truck[7];
	//1.
	readFrompipe(pipefd,truck);
	
	//2.
	int amount;
	printf("Signal arrived------%s\n",truck);
	sendMsg(msgID,truck,&amount);
	
	//3.
	writeToShm(amount,sm);
	//4.
	semaphore_operation(semID,1); // Up
	
	
    int status;
    wait(&status);
	shmctl(sh_mem_id,IPC_RMID,NULL);
	semaphore_delete(semID);
    printf("Parent process ended\n");
  }
  else  //child
  {
    printf("Child send signal to parent. \n"); 
    kill(getppid(),SIGTERM); 
	//1.
    writeTopipe(pipefd);
	
	//2.
	sleep(1);
	readMsg(msgID);
	
	//3.
	semaphore_operation(semID,-1); // down, wait if necessary
	readFromShm(sm);
	semaphore_operation(semID,1); // up 
	
    printf("Child process ended\n");  
  }
  return 0;
}