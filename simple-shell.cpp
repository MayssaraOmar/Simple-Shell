#include <bits/stdc++.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

using namespace std;

//global variables
ofstream logFile;
unordered_set<string> built_in_commands;
#define MAX_ARG_SIZE 100

void SIGCHLD_handler(int sig){
	int status;
	pid_t terminated_child_ID = waitpid(-1,&status,WNOHANG); //to free terminated child
	if(terminated_child_ID > 0){ //Background process / zombie process
		logFile <<"Child process with ID " << terminated_child_ID <<" was terminated\n";
		if(!WIFEXITED(status)){
			cerr <<"The process didn't exit normally\n";
		}
	}
	return;
}

//command class
class command{
	string line;
	char* lineSplitted[MAX_ARG_SIZE];
	bool background;
public:
	//constructor
	command(string &line){
		this->line = line;
		background = false;
		parse();
	}
	//execute command
	void execute(){
		string com = lineSplitted[0];
		if(built_in_commands.find(com) != built_in_commands.end()){ //command is a built in command
			return executeBuiltin();
		}
		return executeOther();
	}
private:
	//parse command line
	void parse(){
		istringstream iss(line);
		int i = 0;
		//split command line based on spaces
		for(string l; iss >> l;){
			lineSplitted[i] = new char[l.size()+1];
			strcpy(lineSplitted[i++], l.c_str());
		}
		lineSplitted[i] = NULL;
		//checks for "&" at the end of the command
		if(!strcmp(lineSplitted[i-1], "&")){ //background process
			background = true;
			delete(lineSplitted[i-1]); //delete "&" from command arguments
			lineSplitted[i-1] = NULL;
		}
	}
	//execute built-in commands ex: cd (commands NOT executed with execvp)
	void executeBuiltin(){
		string com = lineSplitted[0];
		if(com == "cd"){
			int rc = chdir(lineSplitted[1]);
			if (rc < 0) { //error
				cerr << "no such file or directory" << endl;
			}
			return;
		}
		if(com == "exit"){
			exit(0); //exit parent
		}
	}
	//execute non built-in commands (commands executed with execvp)
	void executeOther(){
		pid_t pid = fork(); //create new child process to execute command

		if(pid < 0){ //error
			cerr << "couldn't create new child process\n";
			return;
		}
		if(pid == 0){ //child
			if(background){
				setpgid(0,0); //puts background child process into a new process group(the new group ID (PGID) is equal to the current child ID)
			}
			if(execvp(lineSplitted[0], lineSplitted) < 0){
				//error occurred while executing command
				perror("execvp");
			}
			exit(0); //exits child
		}
		else{ //parent
			//foreground command
			if(!background) {
				int status;
				//parent waits for child to terminate
				pid_t terminated_child_ID = wait(&status);
				//write terminated child process ID in log file
				logFile <<"Child process with ID " << terminated_child_ID <<" was terminated\n";

				if(!WIFEXITED(status)){ //error
					cerr <<"The process didn't exit normally\n";
				}
			}
	}
}
};
//prints shell line
bool printShellLine(){
	char cwd[1024];
	getcwd(cwd, sizeof(cwd)); //gets current directory path
	cout << "Shell:" << cwd <<"$ ";
	return true;
}
//inserts built-ins commands (ex: cd command) in the built_in_commands set
void setBuiltins(){
	built_in_commands.insert("cd");
	built_in_commands.insert("exit");
}

int main(){
	//set signal
	if(signal(SIGCHLD, SIGCHLD_handler)<0){
		//error
		perror("Signal Error");
	}
	setBuiltins();
	logFile.open("logFile.txt"); //open log file to write terminated child process
	string cmline;
	while(printShellLine() && getline(cin, cmline)){
		if(cmline.empty()) {
			continue;
		}
		command cm(cmline);
		cm.execute();
	}
	logFile.close(); //close log file
	return 0;
}
