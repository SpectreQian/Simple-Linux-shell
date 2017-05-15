# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <pwd.h>
# include <sys/wait.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>

# define MAX_SUB_COMMANDS 5
# define MAX_ARGS 10

struct SubCommand
{
        char * line;
        char * argv[MAX_ARGS];
};

struct Command
{
        struct SubCommand sub_commands[MAX_SUB_COMMANDS];
        int num_sub_commands;
        int background;
		int RedirectInFlag;
		int RedirectOutFlag;
};

void ReadArgs(char*,char**,int);
void PrintArgs(char**);
void ReadCommand(char*,struct Command*);
void InviteCommand(void);
void LoopPipe(struct Command * );
void WelScreen(void);
void Background(struct Command * );
int OrderDection(struct Command * );
void Execution(struct Command * );
int Redirection (struct Command *);

int main(int argc, char** argv)
{
        char s[200];
		int pid;
		struct Command command;
		command.RedirectInFlag = 0;
		command.RedirectOutFlag = 0;
		command.background = 0;
		WelScreen();

		int id[2];
		int p;
		
		while(1)
		{
			InviteCommand();
			fgets(s,sizeof s,stdin);
			if(strcmp(s,"\n")==0)		
			{
				if((pid=waitpid(-1,NULL,WNOHANG))>0)
				{
					printf("[%d] finished\n",pid);
					continue;
				}
				else
					continue;
			}
			pipe(id);
						

			ReadCommand(s,&command);
			OrderDection(&command);
			Background(&command);
			
			pid = fork();
			if(pid==0)
			{
				close(id[0]);
				p = getpid();
				write(id[1],&p,sizeof(p));


				if(command.num_sub_commands == 1)
					{
						if(Redirection(&command)== 0)
						{
							if(OrderDection(&command)== 0)
								Execution(&command);	
						}
						else
							execvp(command.sub_commands[0].argv[0],command.sub_commands[0].argv);

					}
				else
				{
					Redirection(&command);
					LoopPipe(&command);
				}
				
				exit(1);
			}
			else
			{
				if(command.background==1)
				{
					close(id[1]);
					read(id[0],&p,sizeof(p));
					printf("[%d]\n",p);
					
					command.background = 0;
				}
				else
				{
					waitpid(pid,NULL,0);
				}
			}
		}
        return 0;
}

void ReadCommand(char*line,struct Command*command)
{
        const char * delim = "|\n";
        char * p;
        char * l;
        int i = 0;
        command->num_sub_commands = 0;
        char * k = strtok(line,delim);

        while(k)
        {
                l = strdup(k);
                command->sub_commands[i].line = l;
                if(i+1 < MAX_SUB_COMMANDS)
                {
                        i++;
                        command->num_sub_commands++;
                        k = strtok(NULL,delim);

                }
                else break;

        }


        for(i=0;i<command->num_sub_commands;i++)

        {
                p = strdup(command->sub_commands[i].line);
                ReadArgs(p,command->sub_commands[i].argv,MAX_ARGS);
        }
}

void ReadArgs(char*in,char**argv,int size)
{
        const char * delim = " \n";
        int j = 0;
        char * k = strtok(in,delim);
        char * l;
        while(k)
        {
                l = strdup(k);
                argv[j] = k;

                if( j+1 < size) j++;
                else break;
                k = strtok(NULL,delim);

        }
        argv[j] = NULL;
}

void InviteCommand (void)
{
        struct passwd * my_info;
        my_info=getpwuid(getuid());
        printf("User [%s] commands:~$ ",my_info->pw_gecos);
}
void LoopPipe(struct Command * command)
{
        int n;
        int fds_to_odd[2];
		int fds_to_even[2];
        int pid;
        
        for(n=0;n<command->num_sub_commands;n++)
        {
				if(n % 2 != 0)
					pipe(fds_to_odd);
				else
					pipe(fds_to_even);
				
				pid = fork();
				if(pid == -1)
                        perror("fork wrong.\n");
                else if (pid == 0)
                {
					if(n == 0)
						dup2(fds_to_even[1],1);
					else if(n == command->num_sub_commands-1)
						{
							if(command->num_sub_commands % 2 !=0)
								dup2(fds_to_odd[0],0);
							else
								dup2(fds_to_even[0],0);
						}
					else
					{
						if (n % 2 != 0)
						{
							dup2(fds_to_even[0],0);
							dup2(fds_to_odd[1],1);
						}
						else
						{
							dup2(fds_to_odd[0],0);
							dup2(fds_to_even[1],1);
						}
					}
					
					execvp(command->sub_commands[n].argv[0],command->sub_commands[n].argv);
                    printf(" %s Command not found\n",command->sub_commands[n].argv[0]);
					exit(1);
						
                }
                else
                {
					if(n==0)
						close(fds_to_even[1]);
					else if(n == command->num_sub_commands-1)
					{
						if (command->num_sub_commands % 2 !=0)
							close(fds_to_odd[0]);
						else
							close(fds_to_even[0]);
						if(command->RedirectOutFlag)
							execvp(command->sub_commands[n].argv[0],command->sub_commands[n].argv);
					}
					else
					{
						if(n % 2 != 0)
						{
							close(fds_to_even[0]);
							close(fds_to_odd[1]);
						}
						else
						{
							close(fds_to_odd[0]);
							close(fds_to_even[1]);
						}
					}
					waitpid(pid,NULL,0);
                }
        }
}

void WelScreen(void)
{
        printf("\n\t********************************************\n");
        printf("\t*      Operating Ststem Mid-term Project   *\n");
        printf("\t********************************************\n");
        printf("\t*         Jing Qian & Xinyi Zhang          *\n");
        printf("\t********************************************\n");
		printf("\t*         Welcome to this Shell!           *\n");
        printf("\t********************************************\n");
        printf("\n");
}

void Background(struct Command * command)
{
	int b;
	
	for(b=0;command->sub_commands[command->num_sub_commands-1].argv[b]!=NULL;b++)
	{
		if(strcmp(command->sub_commands[command->num_sub_commands-1].argv[b],"&") == 0)
			{
				command->background = 1 ;
				command->sub_commands[command->num_sub_commands-1].argv[b] = NULL;
				break;
			}
	}
	
}        
int OrderDection(struct Command * command)
{
	if(strcmp(command->sub_commands[0].argv[0],"exit") == 0)
		exit(0);
	else if (strcmp(command->sub_commands[0].argv[0],"cd") == 0)
	{
		if(command->sub_commands[0].argv[1]==NULL)
		{
				chdir(getenv("HOME"));
				return 1;
		}
		else
		{
			int err = chdir(command->sub_commands[0].argv[1]);
			return 1;
		}
	}
	return 0;
}
void Execution(struct Command * command)
{
	int i;
	for(i=0;i<command->num_sub_commands;i++)
	{
		if(execvp(command->sub_commands[i].argv[0],command->sub_commands[i].argv)<0) 
					fprintf(stderr, "%s : Command not found\n", command->sub_commands[i].argv[0]);
					
	}
}
int Redirection (struct Command * command)
{
	int n,m;

	for(n=0;n<command->num_sub_commands;n++)
		for(m=0;command->sub_commands[n].argv[m]!=NULL;m++)
		{
			if(n==0 && (strcmp(command->sub_commands[n].argv[m],"<")==0))
			{
				if(command->sub_commands[n].argv[m+1]!=NULL)
				{
					close(0);
					int rd = open(command->sub_commands[n].argv[m+1],O_RDONLY);
					command->sub_commands[n].argv[m] = NULL;
					if(rd == -1)
						fprintf(stderr,"%s : File not found\n",command->sub_commands[n].argv[m+1]);
					command->RedirectInFlag = 1;
				}
				else
				{
					printf("Object argument unfound!\n");
					command->RedirectInFlag = 1;
				}
			}
			else if(strcmp(command->sub_commands[n].argv[m],">")==0)
			{
				if(command->sub_commands[n].argv[m+1]!=NULL)
				{
					close(1);
					int wr = open(command->sub_commands[n].argv[m+1],O_WRONLY|O_CREAT,0660);
					command->sub_commands[n].argv[m] = NULL;
					if(wr == -1)
						fprintf(stderr,"%s : Cannot create file\n",command->sub_commands[n].argv[m+1]);
					command->RedirectOutFlag = 1;
				}
				else
				{
					printf("Object argument unfound!\n");
					command->RedirectOutFlag = 1;
				}
			}
		}
		return command->RedirectInFlag;

}

