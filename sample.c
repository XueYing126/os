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

struct messg { 
     long mtype;
     char mtext [ 1024 ]; 
};


void handler(int signumber){
  printf("***Signal %i has arrived for %i!***\n", signumber, getpid());
}

void writeTopipe(int num,int *pipefd){
	close(pipefd[0]); 	
	int i,t;
	for(t=0;t<num;++t){
		i=rand()%1000;
		//printf("Sending voter %i : %i\n", (t+1),i);			
		write(pipefd[1],&i,sizeof(i)); //writes to the pipe
        }	
    close(pipefd[1]);  
	fflush(NULL); 
}

void readFrompipe(int num,int *pipefd,int* voters){
	close(pipefd[1]);  
	printf("I am checking %i Voters!\n",num);
	int i,n;
	for(i=0;i<num;++i)
	{
		read(pipefd[0],&n,sizeof(n));
		printf("Sending voter %i : %i\n",(i+1), n);
		voters[i] = n;
	}
	close(pipefd[0]);
}

void fifosend(int num,int* voters){
	int remark,status,i;
	bool cannotVote;
	char rem[13];
	int f=open("namedpipe",O_WRONLY);
	 for(i=0;i<num;++i)
	{
     write(f,&voters[i],sizeof(voters[i]));
	 
	 bool cannotVote = rand() % 100 < 20;
	
	//„can vote”/”can not vote” 
	if(cannotVote)
		strcpy(rem,"can not vote");
	else
		strcpy(rem,"can vote    ");
	
	  write(f,rem,strlen(rem)+1);
	}
	close(f);
}

void fifoRecieve(int num,int* canVoters,int* realNum){
	int vo,i;
	int j=0;
	char sz[13];
	char str1[13];
	int f=open("namedpipe",O_RDONLY);
	
	
	for(i=0;i<num;++i){
		read(f,&vo,sizeof(vo));
		read(f,&sz,sizeof(sz));
		printf("Second child read data: %i  %s \n",vo,sz);

		strcpy(str1,sz);
		if(strcmp(str1,"can vote    ")==0){
			canVoters[i] = vo;
			++*realNum;
			
		}else{
			canVoters[i] = 0;
		}
	}
	
	close(f);
	unlink("namedpipe");
}

int initMsg(char* argv[]){
    key_t key = ftok(argv[0],1); 
    int msgID = msgget( key, 0600 | IPC_CREAT ); 
    if (msgID < 0) {  perror("msgget error");  exit(EXIT_FAILURE); }
	
	return msgID;
}



void sendMsg(int msgID,int num,int * canVoters){
	//A random number between 1..6
	int idNum,randnum,i,status;
	struct messg m;
	m.mtype = 5;
	char  buf[1024];
	
	i = 0;
	printf("Second child send to message queue... \n");	
	for(i=0;i<num;++i){
		
		idNum = canVoters[i];
	if(idNum!=0){
		
		randnum = rand() % 6 + 1;
		
		//convert in to string: 
		snprintf(buf, sizeof(buf), "%d %d", idNum, randnum);
		strcpy(m.mtext, buf);
		status = msgsnd(msgID,&m, (strlen ( m.mtext ) + 1), 0 );
		
		printf("write to message queue : %s  \n", m.mtext);
		
		if ( status < 0 ) {  perror("msgsnd error");  exit(EXIT_FAILURE); }
		
		}
	}
	
}

void readMsg(int msgID,int n){
    int status,i;
	struct messg m; 	
 
 for(i=0;i<n;++i){
	 
    status = msgrcv(msgID, &m, 1024, 5, 0 ); 
    if ( status < 0 ) {  perror("msgrcv error");  exit(EXIT_FAILURE); }
	
	printf("Parent read from message queue: %s  \n", m.mtext);	
 }

 //destroy msg queue
 if ( msgctl( msgID, IPC_RMID, NULL ) < 0 ) 
	 {  perror("msgctl error");  exit(EXIT_FAILURE); }
 
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
  int status;
  if (argc!=2) {perror("Give a command line argument\n");exit(1);}
  int num = atoi(argv[1]); 
  signal(SIGTERM,handler); 
  
  //unnamed pipe for parent and child 1:
  int pipefd[2]; 
  if (pipe(pipefd) == -1) {perror("Opening error!"); exit(EXIT_FAILURE);}
  
  
  //named pipe between child 1 and 2 :
  int f=mkfifo("namedpipe",0600);
  if (f<0){perror("error");exit(1);}
   
  //create Messagequeue:
  int msgID = initMsg(argv);
	
  //create semaphore:
  int semID = semaphore_create(argv[0],0);  
  
   //unnamed pipe for parent and child 2:
  int pipefd2[2]; 
  if (pipe(pipefd2) == -1) {perror("Opening error!"); exit(EXIT_FAILURE);}
  
  
  
  //create process:
  pid_t first_child=fork();  
  if (first_child<0){perror("The fork calling was not succesful\n"); exit(1);}
  
  if (first_child==0)  //----------in the first_child process-----------------------
  {
	
	printf("I am the checker!\n");    
	sleep(1);
	printf("Child 1 ready!\n");
    kill(getppid(),SIGTERM); 
	
	int voters[num];
	sleep(2);
	
	//1. read from unnamed pipe and write to screen:	
	readFrompipe(num,pipefd,voters);
	
	//2. write to named pipe. 
	fifosend(num,voters);
	
	//free(voters);
	//pause();
	//sleep(4);
    //printf("Child 1 process ended\n"); 
	
  }   //----------------end of  the first_child process-----------------------
  else
  { 
	pid_t sencond_child = fork();
	if (sencond_child<0){perror("The fork calling was not succesful\n"); exit(1);}
	
	if(sencond_child==0){  //----in the sencond_child process-----------------------
		//in sencond_child process
		printf("I am the sealer!\n");
		sleep(2);
		printf("Child 2 ready!\n");
		kill(getppid(),SIGTERM);
		
		sleep(3);
		
		int canVoters[num];
		int realNum = 0;
		//2.  The second child read from named pipe
		fifoRecieve(num,canVoters,&realNum);
		//3. The second child writes to a messagequeue 
		
		//first send the number.
		close(pipefd2[0]); 
		write(pipefd2[1],&realNum,sizeof(realNum)); 
        close(pipefd2[1]);
		
		
		sendMsg(msgID, num, canVoters);
		semaphore_operation(semID,1); // Up
		printf("finished writing,semaphore up \n");
		//free(canVoters);
		//pause();
		//sleep(3);
		//printf("Child 2 process ended\n");
      //-------end of  the sencond_child process-----------------------
	}
	
	else      //-------------in the parent process-----------------------
		       
	{
		printf("I am the president!\n");
		sleep(0.5);
		
		//The president waits for the signals of each of the children 
		//that they are ready for work.
		
		printf("Waiting for Child 1.\n");
		pause();
		printf("Child 1 arrived!\n");
		
		printf("Waiting for Child 2.\n");
		pause();
		printf("Child 2 arrived!\n");
		
		
		printf("\nCount of voters : %i \n", num);
		//1. The presidents write the numbers into an unnamed pipe
		writeTopipe(num,pipefd);
		
		//3. The president reads the messageqeue 
		semaphore_operation(semID,-1); // down, wait if necessary
		
		close(pipefd2[1]);  
	    int n;
		read(pipefd2[0],&n,sizeof(n)); 
		printf("read the num of good voter  : %i \n",n);
        close(pipefd2[0]); 
		
		printf("now parent can read from queue!\n");
        readMsg(msgID,n);  
        semaphore_operation(semID,1); // up 

		//kill(first_child,SIGTERM);
		//kill(sencond_child,SIGTERM);
		//waitpid(first_child,&status,0);
		//waitpid(sencond_child,&status,0);
		semaphore_delete(semID);
		printf("Parent process ended\n");
		   //----------------end of  the parent process-----------------------
	}
  }
  
  return 0;
}