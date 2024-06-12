#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>


/*이 프로그램은 정해진 시간내에 주어진 문장을 정확히 입력하는 프로그램입니다. 
먼저 처음에 원본 텍스트 파일의 전체 크기가 출력됩니다.
그 후 사용자에게 문장이 주어지면 그 문장을 정확히 입력하면 1점을 얻고 틀렸다면 0점을 얻습니다.
주어진 시간이 지나거나 사용자가 원본 텍스트를 모두 입력하면 원본 텍스트와 사용자가 입력한 문자를 전체 비교하는 파일을 출력합니다.
마지막엔 현재 시간이 출력됩니다.
타이핑 연습을 위한 프로그램이라고 생각하시면 됩니다.*/

char *output[] = {"%G년 %m월 %d일 %H:%M"}; //마지막에 현재 시간을 출력하기 위함

struct mymsgbuf{
    long mtype;
    int number;
};

int main(){
    FILE *rfp, *wfp; //읽어올 파일과 내용을 쓸 파일
    char buffer[256]; //공유메모리 쓸 때 사용할 버퍼

    int status; //부모프로세스가 자식프로세스를 기다리기 위한 변수
    
    struct stat statbuf; //파일 정보 검색을 위해 stat구조체 선언

    key_t keyformq;  //메지시 큐를 위한 변수들
    int msgid;
    struct mymsgbuf mesg;

    key_t keyforSM; //공유메모리를 위한 변수들
    int shmid;
    char *shmaddr;

    struct tm *tm; //마지막에 시간을 출력하기 위한 변수들    
    time_t timep;
    char buf[257];

    FILE *fp;
    char buffer2[256]; //파이프에 사용할 버퍼

    stat("text.txt",&statbuf); //파일정보활용
    printf("파일 크기는 is %dbytes입니다. 준비하세요!\n", (int)statbuf.st_size); //원본파일의 크기 출력

    keyformq = ftok("keyfile", 1); //메시지 큐 만들기
    msgid = msgget(keyformq, IPC_CREAT|0666);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }
    
    keyforSM = ftok("shmfile", 65); // 공유 메모리 만들기
    if ((shmid = shmget(keyforSM,1024,IPC_CREAT | 0666)) == -1) {
        perror("shmget");
        exit(1);
    }
    if ((shmaddr = shmat(shmid, NULL, 0)) == (char *)-1) { //공유 메모리 연결하기
        perror("shmat"); 
        exit(1);
    }


    pid_t pid = fork(); //자식프로세스 생성
    if (pid == -1) {  //오류처리
        perror("fork");
        exit(1);
    } else if(pid == 0){ //자식프로세스 정상처리
        if(execlp("./child.out", "child", (char *)NULL) == -1){ //exe함수군으로 child.out 실행
            perror("execlp");
            exit(1);
        }
    } else{
        wait(&status);
        if ((rfp = fopen("text.txt", "r")) == NULL) { //원본 텍스트 파일, 파일출력
            perror("fopen: text.txt");
            exit(1);
        }

        if ((wfp = fopen("result.txt", "w")) == NULL) { //원본 텍스트와 사용자 입력 텍스트를 번갈아서 입력할 파일,파일입력
            perror("fopen: result.out");
            exit(1);
        }

        char *line = strtok(shmaddr, "\n"); //strtok를 통해 공유메모리 내용을 한줄씩 얻기
        while (line != NULL) { //사용자가 입력한 줄까지만 원본파일과 사용자 입력 번갈아서 쓰기

            fgets(buffer, sizeof(buffer), rfp); // fgets를 이용해 원본텍스트 한 줄 얻기
            fprintf(wfp, "%s",  buffer); //원본텍스트 먼저 한 줄 목적지 파일에 쓰기
            
            fprintf(wfp,"%s\n",line); //그 다음 사용자가 입력한 텍스트 한 줄 쓰기
            line = strtok(NULL, "\n"); //공유메모리 내용 다음줄로
            
            fprintf(wfp,"\n");  //한줄 띄우기          
        }

        shmdt(shmaddr); //공유메모리 분리 및 닫기
        shmctl(shmid, IPC_RMID, NULL); 

        fclose(rfp); //두 파일 닫기
        fclose(wfp);

        
        fp = popen("cat result.txt", "r"); //파이프를 사용하여 result.txt파일 전체 출력 준비
        if (fp == NULL) {
            perror("popen");
            exit(1);
        }

        
        printf("\n");
        printf("Your output is\n");
        while (fgets(buffer2, sizeof(buffer2), fp) != NULL) { //파이프로 전체 출력
            printf("%s", buffer2);
        }
        

        
        if (msgrcv(msgid, &mesg, sizeof(mesg.number), 1, 0) == -1) { //메시지 큐로 점수 얻기
            perror("msgrcv");
        }

        printf("당신의 점수는 %d입니다\n", mesg.number);//점수 출력

        // 파이프 닫기
        pclose(fp);        

        // 메시지 큐 삭제
        if (msgctl(msgid, IPC_RMID, NULL) == -1) {
            perror("msgctl");
        }
        
        //시스템 정보 활용
        time(&timep);  //현재 시간을 출력하기 준비
        tm = localtime(&timep);

        strftime(buf, sizeof(buf), output[0], tm);
        printf("--%s--\n", buf); //현재시간 출력
    }
}