#include<stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/msg.h> 
#include <unistd.h>
#include<stdbool.h>
#include <sys/sem.h>
#include <wait.h>
#include <sys/stat.h>
#include <sys/ipc.h>

struct msg { 
     long mtype;
     char mtext [ 1024 ]; 
}; 


typedef struct
{	
	char address[50];
	double size;
	char list_of_tasks[150];
	//char deadline[15];
	int year;
	int month;
	int day;
}Order;



Order my_orders[30];
static int count = 0;


void syncarray(){ //empty the file,and Synchronize the file with array.
	FILE *file;
	file = fopen("orders.txt","w"); 
	if(file == NULL){
		printf("Fail to open the file\n");
		return;
	}
	int i;
	for (i =0;i<count;i++){  //"\n%d;%s;%-.2lf;%s;%s"
	fprintf(file,"\n%s;%-.2lf;%s;%d;%d;%d",my_orders[i].address,my_orders[i].size,my_orders[i].list_of_tasks,
	my_orders[i].year,my_orders[i].month,my_orders[i].day);
	}
	
	fclose(file);
}



void ListOrders(){
	int i;
	printf("\n---------Now we list all the orders : \n");
	
	for (i =0;i<count;i++){		
	printf("For the %i th order : \n",i+1);
	printf("Address: %-30s\n",my_orders[i].address);
	printf("Size (in m2): %-10.2lf\n",my_orders[i].size);
	printf("Tasks: %-50s\n",my_orders[i].list_of_tasks);
	printf("Deadline: %i %i %i \n\n",my_orders[i].year,my_orders[i].month,my_orders[i].day);		
	}
	
}

void NewOrder(){
	Order new_order;
	printf("\n---------Give us the information of your order: \n\n");
	printf("Enter address of the house: \n");
	scanf(" %[^\n]",&new_order.address);
	printf("Enter size of the house(m2): \n");
	scanf("%lf",&new_order.size);
	printf("Enter list of tasks(e.g. painting, electrical installation, etc.): \n");
	scanf(" %[^\n]",&new_order.list_of_tasks);
	printf("Enter deadline:\n");
	//scanf(" %[^\n]",&new_order.year,);
	printf("year (yyyy):\n");	
	scanf("%d",&new_order.year);
	printf("month (mm):\n");	
	scanf("%d",&new_order.month);
	printf("day (dd):\n");	
	scanf("%d",&new_order.day);
	//now we add new_order to the array my_orders
	my_orders[count] = new_order;
	count++;
	syncarray();

}
void ModifyOrder(){
	
	printf("------Which order do you want modify? please give me the address of it \n");
	char tmp_address[30];
	scanf(" %[^\n]",&tmp_address);
	int i;
	int exist = 0;
	for (i =0;i<count;i++){
		if (strcmp(my_orders[i].address, tmp_address)==0){	
			exist = 1;			
			
			printf("For this order : \n",i);
			printf("Address: %-30s\n",my_orders[i].address);
			printf("Size (m2): %-10.2lf\n",my_orders[i].size);
			printf("Tasks : %-50s\n",my_orders[i].list_of_tasks);
			printf("Deadline (yyyy mm dd): %i %i %i \n\n",my_orders[i].year,my_orders[i].month,my_orders[i].day);

	//printf("\n---------Give us the new information of your order: \n\n");
	printf("\n Enter new address of the house: (if do not want modify , just repeat it.) \n");
	scanf(" %[^\n]",&my_orders[i].address);
	printf("Enter new size of the house(m2): \n");
	scanf("%lf",&my_orders[i].size);
	printf("Enter new list of tasks(e.g. painting): \n");
	scanf(" %[^\n]",&my_orders[i].list_of_tasks);
	printf("Enter new deadline: \n");
	printf("year:\n");	
	scanf("%d",&my_orders[i].year);
	printf("month:\n");	
	scanf("%d",&my_orders[i].month);
	printf("day:\n");	
	scanf("%d",&my_orders[i].day);
	//scanf("%d %d %d",&my_orders[i].year,&my_orders[i].month,&my_orders[i].day);		
		}
	}
	if(exist == 0){printf("\nno such address \n\n");}
	else{ syncarray();}   //Synchronize the file
}
void DeleteOrder(){
	printf("------Which order do you want delete? please give me the address of it \n\n");
	char tmp_address[30];
	scanf(" %[^\n]",&tmp_address);
	int i;
	int exist = 0;
	for (i =0;i<count;i++){
		//if (my_orders[i].address==tmp_address){	is not good
		if (strcmp(my_orders[i].address, tmp_address)==0){
			int pos;
			for(pos = i; pos<count-1;pos++){
				my_orders[pos] = my_orders[pos+1] ;
			}
			count--;	
			exist = 1;
			printf("Successful Delete \n\n");
		}
		
	}
	if(exist == 0){printf("\nno such address \n\n");}
	else{syncarray();} //  //Synchronize the file
}
void Delete(int index){
	int pos;
	if(index<count){
		for(pos = index;pos<count-1;++pos){
			my_orders[pos] = my_orders[pos+1] ;
		}
		count--;
		syncarray();	
	}else{printf("\nno such order \n\n");}
}


void initarray(){ //add to array the content in file
	FILE *file;	
	file = fopen("orders.txt","r");
	
	if(file != NULL){
		Order tmp_order;	
	while(1){
		if(feof(file)){ break;}	
		fscanf(file,"%[^;];%lf;%[^;];%d;%d;%d\n",&tmp_order.address,&tmp_order.size,
		&tmp_order.list_of_tasks,&tmp_order.year,&tmp_order.month,&tmp_order.day);	
		my_orders[count] = tmp_order;
		count++;		
	}
	
	printf("\n");
	fclose(file);
	}
}
int dateToInt(int index){
	int y = my_orders[index].year;
	int m = my_orders[index].month;
	int d = my_orders[index].day;
	return 365*y+30*m+d;
}

int SDF(){
	int i;
	int index = 0;
	for (i =1;i<count;i++){
		if(dateToInt(i)<dateToInt(index))
			index = i;
	}
	return index;
}


void handler(int signumber){
  printf("\n");
}

void send(int mqueue) 
{     
	int remark,status;
	remark =rand()%100,status;
	struct msg m1 = { 5, "success" }; 
	struct msg m2 = { 5, "missed" };

	 if(remark<10){
		 status = msgsnd(mqueue,&m2, strlen ( m2.mtext ) + 1 , 0 ); 
		 printf( "Team sent to message queue :  %s\n", m2.mtext );
	 }else{
		status = msgsnd(mqueue, &m1, strlen ( m1.mtext ) + 1 , 0 ); ;
		printf( "Team sent to message queue :  %s\n", m1.mtext );
	 }

 
     if ( status < 0 ) 
          perror("msgsnd error"); 

} 
     
// receiving a message. 
void receive(int mqueue) 
{ 

     struct msg m;
     int status; 
 
     status = msgrcv(mqueue, &m, 1024, 5, 0 ); 
     if ( status < 0 ) 
          perror("msgsnd error"); 
     else
          printf( "Firm read from message queue :  %s\n", m.mtext); 
}

int initMsg(char* argv[]){
	int msgID; 
    key_t key; 
    key = ftok(argv[0],1); 
    msgID = msgget( key, 0600 | IPC_CREAT ); 
    if (msgID < 0) {  perror("msgget error");  exit(EXIT_FAILURE); }
	
	return msgID;
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


int main(int argc, char* argv[])
{ 
initarray();
int option,status;
do{
   printf("\n1--List orders\n2--New Order\n3--Modify order\n4--Delete order\n5--Start to work! \n\n");
   scanf("%d",&option);
   switch(option){
	case 1:
		ListOrders();
		break;
   	case 2:
   		NewOrder();
		break;
	case 3:
		ModifyOrder();
		break;
	case 4:
		DeleteOrder();
		break;
	case 5:
		break;
	default:
		printf("\nWrong option.\n");
	}
	
  }while(option !=5);
  
   signal(SIGTERM,handler); //handler = SIG_IGN - ignore the signal (not SIGKILL,SIGSTOP), 
                           //handler = SIG_DFL - back to default behavior 

  //unnamed pipe:
  int pipefd[2]; 
  if (pipe(pipefd) == -1) {perror("Opening error!"); exit(EXIT_FAILURE);} 
  
  //message equeue
    int msgID = initMsg(argv); 
	
  //semaphore_create
  int semID = semaphore_create(argv[0],0); // sem state is down!!!
  
  pid_t child=fork();
  if (child<0){perror("The fork calling was not succesful\n"); exit(1);}
  
  if (child>0)
  { 
    pause();  
	
	int chosen = SDF();
	printf("Firm sends the data of the chosen (SDF) recovery %i to the team: \n",(chosen+1));
    close(pipefd[0]); 
		write(pipefd[1],&chosen,sizeof(chosen)); 
    close(pipefd[1]);
    
	semaphore_operation(semID,-1); // down, wait if necessary
    receive(msgID);  
    semaphore_operation(semID,1); // up      
    

	kill(child,SIGTERM);	
    int status;
	waitpid(child,&status,0);
	fflush(NULL);
	status = msgctl( msgID, IPC_RMID, NULL ); 
    if ( status < 0 ) 
        perror("msgctl error"); 
	semaphore_delete(semID);
    printf("Parent process ended\n\n");

  }
  else 
  {
	int status;
    printf("Working team ready to start! send to firm signal: %i\n",SIGTERM);
    kill(getppid(),SIGTERM); 
    sleep(1);
	close(pipefd[1]);  
	int n;
		read(pipefd[0],&n,sizeof(n)); 
		printf("Child read the chosen recovery : %i \n",n+1);
	
    close(pipefd[0]); 
	Delete(n);
	
	send(msgID);
	semaphore_operation(semID,1); // Up
	
	
	pause();
    printf("Child process ended\n");  
  }
  return 0;
}

