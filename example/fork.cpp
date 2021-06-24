#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

int main(){
  pid_t pid  = fork();
  if(pid < 0){
    cout<<"fork failure!!!"<<endl;
  }else if(pid == 0){ // 子进程
    cout<<"this is child process"<<endl;
    exit(0);
  }else{
    cout<<"this is parent process"<<endl;
    wait(0);
  }
  return 0;
}