// this is some text from mauryShell :D
// this is so cool im using my own shell to write into mauryShell.c using GNU nano
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_ARGS    16

bool searchFolder(const char path[], const char fileName[]);
void commandEcho(const char buffer[]);
void commandType(const char buffer[], const char builtIns[], char allPaths[]);
void commandPwd(void);
void commandCd(const char buffer[]);
void searchCommand(char buffer[], char allPaths[], const char command[]);
char* parseNextSQ(const char **cursor, int *isQuoted, int* hadWhitespace);

int main() {
  char buffer[128];
  char *command;
  char builtIns[] = {"exit echo type pwd cd"};

  char *pathValue = getenv("PATH");

  // flush after every printf
  setbuf(stdout, NULL);

  while(1)
  {
    printf("mauryShell $: ");

    fgets(buffer, sizeof(buffer), stdin);
    
    buffer[strcspn(buffer, "\n")] = '\0'; // find first occurrence of '\n' and replace with '\0'
    
    // parse the command name (first word, not using strtok to preserve the rest)
    char commandName[128] = "";
    const char *s = buffer;
    
    // skip leading whitespace
    while(*s && isspace(*s))
      s++;
    
    // Extract command name (up to first space or quote)
    int i = 0;
    while(*s && !isspace(*s) && *s != '\'')
    {
      commandName[i++] = *s++;
    }
    commandName[i] = '\0';
    
    command = commandName;

    // create a copy of our PATH to prevent corrupting our environment memory
    char allPaths[strlen(pathValue) + 1];
    strcpy(allPaths, pathValue);

    // Start evaluating each valid command
    if(strcmp(command, "exit") == 0) // exit command
      return 0;
    else if(strcmp(command, "echo") == 0) // echo command
    {
      commandEcho(buffer);
      continue;
    }
    else if(strcmp(command, "type") == 0) // type command
    {
      commandType(buffer, builtIns, allPaths);
      continue;
    }
    else if(strcmp(command, "pwd") == 0) // pwd command
    {
      commandPwd();
      continue;
    }
    else if(strcmp(command, "cd") == 0) // cd command
    {   
      commandCd(buffer); // second argument in our buffer will contain the path
    } 
    else
      searchCommand(buffer, allPaths, command); // search for command and pass its arguments
  }
  
  return 0;
}

bool searchFolder(const char path[], const char fileName[])
{
  char fullPath[strlen(path) + strlen(fileName) + 2]; // add 1 for the null terminating character and 1 for the "/"
  snprintf(fullPath, sizeof(fullPath), "%s/%s", path, fileName); // create the full file path

   // use access(fullPath, X_OK) to check if the file is executable
  if(access(fullPath, X_OK) == 0)
    return true;
  else
    return false;
}

void commandEcho(const char buffer[])
{
  char result[1024] = "";
  const char *cursor = buffer + strlen("echo") + 1;
  char *token;
  int isQuoted;
  int hadWhitespace;
  int isFirstToken = 1;
  int whitespaceBeforeEmptyQuote = 0;
  
  while((token = parseNextSQ(&cursor, &isQuoted, &hadWhitespace)) != NULL)
  {
      // empty quoted strings are ignored
      if(isQuoted && strlen(token) == 0)
      {
          free(token);
          whitespaceBeforeEmptyQuote = hadWhitespace;
          continue;
      }
      
      // determine effective whitespace
      int effectiveWhitespace = hadWhitespace || whitespaceBeforeEmptyQuote;
      whitespaceBeforeEmptyQuote = 0;
      
      // add space only if there was whitespace and not first token
      if(!isFirstToken && effectiveWhitespace)
      {
          strncat(result, " ", sizeof(result) - strlen(result) - 1);
      }
      
      strncat(result, token, sizeof(result) - strlen(result) - 1);
      free(token);
      
      isFirstToken = 0;
  }
  
  printf("%s\n", result);
}

void commandType(const char buffer[], const char builtIns[], char allPaths[])
{
  // Skip "type " to get to the argument
  const char *cursor = strstr(buffer, "type") + 4;
  
  // Skip whitespace
  while(*cursor && isspace(*cursor))
    cursor++;
  
  // Extract the type command argument
  char typeCommand[128] = "";
  int i = 0;
  while(*cursor && !isspace(*cursor))
  {
    typeCommand[i++] = *cursor++;
  }
  typeCommand[i] = '\0';
  
  if(strstr(builtIns, typeCommand) != NULL) // search for substring in our builtIns char array
  {
    printf("%s is a shell builtin\n", typeCommand);
    return;
  }

  // if not a built in command then start searching through the paths and search for the typeCommand
  char *currentPath = strtok(allPaths, ":"); // e.g. allPaths = "/usr/bin:/usr/local/bin"

  while(currentPath != NULL)
  {
    // check if typeValue is in first path, otherwise check the next path
    if(searchFolder(currentPath, typeCommand)) // checks if command exists and has execute permissions
    {
      printf("%s is %s/%s\n", typeCommand, currentPath, typeCommand);
      return;
    }
    else
      currentPath = strtok(NULL, ":"); // move to next file path
  }

  printf("%s: not found\n", typeCommand);
}

void commandPwd(void)
{
  char cwd[1024];

  if(getcwd(cwd, sizeof(cwd)) != NULL) // print working directory by using the getcwd() system call
  {
    printf("%s\n", cwd); // print the current working directory
    return;
  }
  else
    printf("error getcwd()\n");
}

void commandCd(const char buffer[])
  {
    const char *path;
    path = buffer + strlen("cd") + 1;

    if(strcmp(path, "~") == 0)
    {
      path = getenv("HOME");
      
      if(path == NULL)
      {
        printf("HOME environment variable is not set\n");
        return;
      }
    }

    if(chdir(path) == -1)
      printf("cd: %s: No such file or directory\n", path);
  }

void searchCommand(char buffer[], char allPaths[], const char command[])
{
  // if command is not a built in command and command is not "type command_exe" then search for "command_exe"
  char *currentPath = strtok(allPaths, ":");

  while(currentPath != NULL)
  {
    if(searchFolder(currentPath, command))
    {
      // handling the case where we get an executable like "command_exe arg1 arg2"
      // parse the entire command variable and pass into execvp(programName, argList) function

      char *args[MAX_ARGS];
      uint8_t index = 1;
      args[0] = (char*)command;

      // parse arguments using the same quote-aware parser as echo
      const char *cursor = buffer + strlen(command);
      
      // skip the space after the command
      while(*cursor && isspace(*cursor))
        cursor++;
      
      char *token;
      int isQuoted;
      int hadWhitespace;
      int whitespaceBeforeEmptyQuote = 0;
      char argBuffer[1024] = "";  // buffer to build each argument
      int isFirstToken = 1;
      
      while((token = parseNextSQ(&cursor, &isQuoted, &hadWhitespace)) != NULL)
      {
        // empty quoted strings are ignored
        if(isQuoted && strlen(token) == 0)
        {
          free(token);
          whitespaceBeforeEmptyQuote = hadWhitespace;
          continue;
        }
        
        // determine effective whitespace
        int effectiveWhitespace = hadWhitespace || whitespaceBeforeEmptyQuote;
        whitespaceBeforeEmptyQuote = 0;
        
        // if there's whitespace, we start a new argument
        if(!isFirstToken && effectiveWhitespace)
        {
          // save current argument
          args[index] = strdup(argBuffer);
          index++;
          argBuffer[0] = '\0';  // Reset buffer
        }
        
        // append token to current argument
        strncat(argBuffer, token, sizeof(argBuffer) - strlen(argBuffer) - 1);
        free(token);
        
        isFirstToken = 0;
      }
      
      // don't forget the last argument if there is one
      if(strlen(argBuffer) > 0)
      {
        args[index] = strdup(argBuffer);
        index++;
      }
      
      args[index] = NULL;  // null-terminate the args array

      pid_t pid = fork();

      if(pid == -1)
      {
        printf("fork failed\n");
        
        // free allocated arguments
        for(int i = 1; i < index; i++)
          free(args[i]);
        
        return;
      }
      else if(pid == 0)
      {
        if(execvp(command, args) == -1) // child process executes the command
        {
          printf("execvp failed\n");
          exit(1);
        }
      }
      else
      {
        int status;
        waitpid(pid, &status, 0);
        
        // free allocated arguments in parent process
        for(int i = 1; i < index; i++)
          free(args[i]);
        
        return;
      }
    }
    else
      currentPath = strtok(NULL, ":");
  }

printf("%s: command not found\n", command);
}

char* parseNextSQ(const char **cursor, int *isQuoted, int* hadWhitespace)
{
  if(cursor == NULL || *cursor == NULL)
    return NULL;

  const char *s = *cursor;
  
  // check if there's leading whitespace
  *hadWhitespace = 0;
  while(*s && isspace(*s))
  {
    *hadWhitespace = 1;
    s++;
  }
  
  if(*s == '\0')
    return NULL;
  
  // check if this is a quoted string
  if(*s == '\'')
  {
    *isQuoted = 1;
    s++; // skip opening quote
    const char *start = s;
    
    // find closing quote
    while(*s && *s != '\'')
        s++;
    
    if(*s == '\0')
    {
        // no closing quote - treat as error or end
        return NULL;
    }
    
    size_t len = s - start;
    char *token = (char*)malloc(len + 1);
    
    if(token == NULL)
        return NULL;
    
    memcpy(token, start, len);
    token[len] = '\0';
    
    *cursor = s + 1; // move past closing quote
    return token;
  }
  else
  {
    // unquoted word we read until space or quote
    *isQuoted = 0;
    const char *start = s;
    
    while(*s && !isspace(*s) && *s != '\'')
        s++;
    
    size_t len = s - start;
    char *token = (char*)malloc(len + 1);
    
    if(token == NULL)
        return NULL;
    
    memcpy(token, start, len);
    token[len] = '\0';
    
    *cursor = s;
    return token;
  }
}
