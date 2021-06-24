#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;
int main(){
  pid_t child2parent[2]; // 子进程项父进程发送数据
  pipe(child2parent);
  pid_t pid  = fork();
  if(pid < 0){
    cout<<"fork failure!!!"<<endl;
  }else if(pid == 0){ // 子进程
    close(child2parent[0]);
    cout<<"send data to parent"<<endl;
    write(child2parent[1],"this msg is from child", 23);
    close(child2parent[1]);
    exit(0);
  }else{
    close(child2parent[1]);
    char buf[50];
    read(child2parent[0],buf, sizeof(buf));
    close(child2parent[0]);
    printf("receive from child:%s\n",buf);
    wait(0);
  }
  return 0;
}