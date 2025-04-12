#include <stdio.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <termios.h>
using namespace std;

//定义进程状态
typedef enum {
    READY,
    RUNNING,
    EXIT,
    BLOCKED
} ProcessState;

//定义PCB结构体
struct PCB{
    int pid;    //进程号
    char name[20];  //进程名字
    ProcessState state; //进程状态
    int total_time; //需要总时间
    int remaining_time; //还需要的时间
    struct PCB* next;   //指向下一个节点的指针
};

//定义全局变量
int next_pid = 1;   //所有进程号顺序
PCB* running_process = NULL;   //正在运行的进程指针
PCB* ready_queue = NULL;   //就绪进程队列
PCB* blocked_queue = NULL; //阻塞进程队列
PCB* free_pcbs = NULL; //可用PCB资源队列
const int TIME_QUANTUM = 3; //定义时间片长度

//初始化空闲PCB队列
void initialize_free_pcbs(int nums){
    for(int i = 0; i < nums; i++){
        PCB* pcb = (PCB*)malloc(sizeof(PCB));
        //头插
        pcb->next = free_pcbs;
        free_pcbs = pcb;
    }
}

//获取一个空闲的pcb资源
PCB* get_free_pcb(){
    if(!free_pcbs){
        cout << "无可用pcb资源" << endl;
        return NULL;
    }
    PCB* free_pcb = free_pcbs;
    free_pcbs = free_pcbs->next;
    free_pcb->next = NULL;
    return free_pcb;
}

//释放pcb到空闲列表
void free_pcb(PCB* pcb){
    //头插，下次再创建进程的时候再覆盖pcb里面的内容
    pcb->next = free_pcbs;
    free_pcbs = pcb;
}

//将指定进程放在就绪队列尾部
void enqueue_ready(PCB* pcb){
    pcb->state = READY;
    //尾插
    if(!ready_queue){
        ready_queue = pcb;
    }else{
        PCB* current = ready_queue;
        for(;current->next;current = current->next);
        current->next = pcb;
    }
}

//创建新进程
void create_new_process(char* name,int total_time){
    PCB* pcb = get_free_pcb();
    if(pcb == NULL) return;

    pcb->pid = next_pid++;
    strcpy(pcb->name,name);
    pcb->total_time = total_time;
    pcb->remaining_time = total_time;
    pcb->next = NULL;

    enqueue_ready(pcb);

    cout << "Create process " << pcb->pid <<" (" << name << ") with total time " << total_time << endl;
}

//从就绪队列里面取下一个进程
PCB* select_next_process(){
    if(!ready_queue) return NULL;
    //头取
    //复制一份
    PCB* pcb = ready_queue;
    ready_queue = ready_queue->next;
    //去掉后面，留第一个
    pcb->next = NULL;
    return pcb;
}

//将某进程放在阻塞进程里面
void enqueue_blocked(PCB* pcb){
    pcb->state = BLOCKED;
    //尾插
    if(!blocked_queue){
        blocked_queue = pcb;
    }else{
        PCB* current = blocked_queue;
        for(;current->next;current = current->next);
        current->next = pcb;
    }
}

//检查阻塞队列是否有进程解除了阻塞
void check_blocked_queue(){
    if(!blocked_queue) return;

    //简单模拟：每次调度时有几率解除一个阻塞
    if(rand() % 3 == 0){    //1/3的概率解除
        PCB* unblocked_process = blocked_queue;
        blocked_queue = blocked_queue->next;
        unblocked_process->next = NULL;
        cout << "Process " << unblocked_process->pid << " (" << unblocked_process->name << ") "" unblocked." << endl;
        enqueue_ready(unblocked_process);
    }
}

//进程调度
void schedule(){
    //检查阻塞队列
    check_blocked_queue();

    //拿掉一个running -> 终止/就绪
    if(running_process){
        if(running_process->remaining_time == 0){
            cout << "Process " << running_process->pid << " (" << running_process->name <<" )" << " has finished excution." << endl;
            free_pcb(running_process);
        }else{
            enqueue_ready(running_process);
        }
    }

    //在就绪队列里面拿一个运行
    running_process = select_next_process();

    if(!running_process){
        cout << "No ready process in ready queue" << endl;
        return;
    }

    running_process->state = RUNNING;
    cout << "Now running process " << running_process->pid << " (" << running_process->name << ")" <<endl;
}

//模拟运行一个时间片（操作运行的程序使其运行一个时间片）
int run_time_slice(){
    //如果当前没有运行的程序
    if(!running_process){
        cout << "No process is running" << endl;
        if(ready_queue == NULL && blocked_queue == NULL){
            cout << "All process have complished" << endl;
            return 0;
        }
        return 1;
    }

    //运行需要运行的程序
    int time_to_run = running_process->remaining_time < TIME_QUANTUM ? running_process->remaining_time : TIME_QUANTUM;
    cout << "Running process " << running_process->pid << " (" << running_process->name << ") " << "for" << time_to_run << " units of time" << endl;
    running_process->remaining_time -= time_to_run;

    //让用户决定是否阻塞该程序或直接退出
    cout << "Press Enter to continue, Esc to block, q to quit..." << endl;

    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    int ch;
    while (1) {
        ch = getchar();
        if (ch == '\n') {  // Enter键
            break;
        } else if (ch == 27) {  // Esc键
            printf("\nProcess %d (%s) blocked\n", running_process->pid, running_process->name);
            enqueue_blocked(running_process);
            running_process = NULL;
            break;
        } else if (ch == 'q') {  // q键退出
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            exit(0);
        }
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 1;
}

//打印进程信息
void print_process_info(PCB* pcb){
    if(pcb == NULL) return;

    cout << "PID: " << pcb->pid << endl;
    cout << "Name: " << pcb->name << endl;
    cout << "State: ";
    switch (pcb->state) {
        case READY: cout << "Ready"; break;
        case RUNNING: cout << "Running"; break;
        case BLOCKED: cout << "Blocked"; break;
        case EXIT: cout << "Exit"; break;
    }
    cout << endl;
    cout << "Remaining time: " << pcb->remaining_time << "/" << pcb->total_time << endl;
    cout << "--------------------" << endl;
}

//打印队列信息
void print_queue_info(PCB* queue,char* queue_name){
    cout << "\n" << queue_name << " Queue:\n"
        << "--------------------\n";
    for(PCB* current = queue; current != nullptr; current = current->next) 
        print_process_info(current);
    
    if(!queue) {
        cout << "(empty)\n";
    }
    cout << "--------------------\n";
}

int main(){
    //初始化PCB空闲列表，分配PCB资源
    initialize_free_pcbs(10);

    //创建一些初始流程
    create_new_process("Process A",8);
    create_new_process("Process B",5);
    create_new_process("Process C",3);
    create_new_process("Process D",6);

    //主循环
    while(1){
        //打印当前状态
        print_queue_info(ready_queue,"Ready");
        if(running_process){
            cout << "Running process: " << endl;
            print_process_info(running_process);
        }
        print_queue_info(blocked_queue,"Blocked");

        schedule();
        if(run_time_slice() == 0) break;
        cout << endl << "====================================" << endl;
    }
    return 0;
}