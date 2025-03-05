#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

char *lsh_read_line(void) {
  size_t bufsize = 0;
  char *line = NULL;
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);
    } else {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }
  return line;
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char **lsh_split_line(char *line) {
  int bufsize = LSH_TOK_BUFSIZE, pos = 0;
  char **tokens = malloc(sizeof(char *) * bufsize);
  char *token;
  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[pos++] = token;
    if (pos >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[pos] = NULL;
  return tokens;
}

int lsh_launch(
    char **args) { // first the parent creates a child , they both get their
                   // "lsh_launch" function and they are handled based on the
                   // return value of fork which is pid. if the pid =0 then
                   // child , if greater than 0 then we are in parent and it
                   // executes until the child is finished (execvp continuously
                   // replaces the child with the next arguement function until
                   // -1 is returned which is the case of an error)
  pid_t pid, wpid;
  int status;

  pid = fork(); // in parent, this returns the childs pid which is newly created
                // and if we are already in the child, fork will return 0
  if (pid == 0) {
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);

  } else if (pid < 0) {
    perror("lsh");
  } else {
    do {
      wpid = waitpid(pid, &status,
                     WUNTRACED); // waitpid sets the status of the child of pid
                                 // in the status variable and wuntraced simply
                                 // helps wait for additional scenarious such as
                                 // the child pausing or stopped which typically
                                 // isnt handled by wifexited or wifsignaled
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 1;
}

int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

char *builtin_str[] = {"cd", "help", "exit"};

int (*builtin_func[])(
    char **) = { // quite literally an array of function pointers taking the
                 // arguements of char ** for each function like lsh_cd(args)
    &lsh_cd, &lsh_help, &lsh_exit};

int lsh_num_builtins() { return sizeof(builtin_str) / sizeof(char *); }

int lsh_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected arguement to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

int lsh_help(char **args) {
  printf("Use the man command for information on other programs.\n");
  return 1;
}

int lsh_exit(char **args) { return 0; }

int lsh_execute(char **args) {
  int i;
  if (args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }
  return lsh_launch(args);
}

void lsh_loop(void) {
  char *line;
  char **args;
  int status;
  do {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}
int main(int argc, char **argv) {
  lsh_loop();
  return EXIT_SUCCESS;
}
