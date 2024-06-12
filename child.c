#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>

volatile sig_atomic_t trigger = 0; // 사용자가 입력받다가 알람시간이 되면 루프 탈출을 위한 변수

void sig_handler() {
    trigger = 1;  //1이되면 탈출
}

struct mymsgbuf{
    long mtype;
    int number;
};

int main(){
    key_t keyforSM; //공유메모리를 위한 변수들
    int shmid;
    char *shmaddr;
    char input[256]; //사용자에게 입력받기 위한 변수

    key_t keyformq;  //메지시 큐를 위한 변수들
    int msgid;
    struct mymsgbuf mesg;

    int fd;  //메모리메핑을 위한 변수들  
    char *addr;
    struct stat statbuf;
    
    int correct_count = 0; //사용자가 정답을 맞출 때 마다 1늘어나는 변수

    keyformq = ftok("keyfile", 1); //메세지큐 키
    keyforSM = ftok("shmfile", 65); // 공유메모리 키

    signal(SIGALRM, sig_handler);
    
    fd = open("text.txt", O_RDONLY); //메모리메핑을 위한 파일 열기
    if (fd == -1) {
        perror("파일 열기 실패");
        exit(1);
    }
    
    if (fstat(fd, &statbuf) == -1) { //메모리메핑을 위한 파일 크기 얻기
        perror("fstat");
        close(fd);
        exit(1);
    }

    // 파일 매핑
    addr = mmap(NULL, statbuf.st_size , PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(1);
    }

    addr[statbuf.st_size] = '\0'; //strtok을 사용하기 위해서 마지막 문자를 null로 바꾸기

    // 메시지 큐 열기
    msgid = msgget(keyformq, 0666);
    if (msgid == -1) {
        perror("msgget");
    }
    
    // 공유 메모리 연결
    if ((shmid = shmget(keyforSM, 1024, 0666)) == -1) {
        perror("shmget");
        exit(1);
    }
    if ((shmaddr = shmat(shmid, NULL, 0)) == (char *)-1) {
        perror("shmat");
        exit(1);
    }

    char *line = strtok(addr, "\n");
    printf("테스트 시작!\n");
    alarm(10); //10초후 종료되게 설정
    while (line != NULL && !trigger) { //원본파일이 모두 출력되거나 trigger가 발생하면 입력 종료
        printf("%s\n", line);        
        if (fgets(input, sizeof(input), stdin) != NULL) { //사용자 입력받기
            strcat(shmaddr, input); //공유메모리에 사용자 입력 이어 붙이기
            input[strcspn(input, "\n")] = '\0';//strcspn함수로 사용자 입력 \n(마지막 문자열)의 인덱스를 찾고 \n을 null문자로 변환
                                               //원본 텍스트와 사용자 입력 비교위해 이 작업 수행
            if (strcmp(line, input) == 0) { //사용자가 정확히 입력했다면 정답 카운트 1증가
                correct_count++;
            }
        }    
        line = strtok(NULL, "\n"); //원본 텍스트 다음줄로
    }

    mesg.number=correct_count; //정답 전달
    mesg.mtype = 1;  // 메시지 유형 설정

    // 메시지 큐에 메시지 전송
    if (msgsnd(msgid, &mesg, sizeof(mesg.number), 0) == -1) {
        perror("msgsnd");
    }

    // 공유 메모리 분리
    shmdt(shmaddr);

    // 메모리 매핑 해제 및 파일 닫기
    munmap(addr, statbuf.st_size);
    close(fd); //메모리메핑 떄 연 파일 닫기
    
}