// Simple grep.  Only supports ^ . * $ operators.
// Alex Brady December, 2015
// Added command line options -c, -i, and started -A
#include "types.h"
#include "stat.h"
#include "user.h"

#define NULL ((void*) 0)

char buf[1024];
int match(char*, char*);

// Function taken from user ryyker
// http://stackoverflow.com/questions/19300596/implementation-of-tolower-function-in-c
// Converts a char* to its lowercase form
char * strtolower(char *str)
{
    char *pNew1 = str;
    char *pNew2 = str;

    if(str != NULL) //NULL ?
    {
        if(strlen(str) != 0) //"" ?
        {
            while(*pNew1)
            {
                *pNew2 = (*pNew1 >='A' && *pNew1<='Z') ? (*pNew1 + 32) : (*pNew1); 
                ++pNew2;
                ++pNew1;
            }
            *pNew2 = '\0';
            return str;// return changed string
        }              // and prevent returning null to caller
    }
    return "";//Will never get here for non-null input argurment
} 

void
grep(char *pattern, int fd, int options[])
{
  // options: [-i, -c, -A, -B, -C] set to 1 if true
  int n, m, counter;
  char *p, *q;
  
  m = 0;
  counter = 0;
  while((n = read(fd, buf+m, sizeof(buf)-m-1)) > 0){
    m += n;
    buf[m] = '\0';
    p = buf;
    // Keep searching for matches until the buffer does not have any more newlines
    while((q = strchr(p, '\n')) != 0){
      *q = 0;
      // options[0]: -i
      if(options[0] == 1){
	if(match(strtolower(pattern), strtolower(p))){
          *q = '\n';
	  // options[1]: -c
          if(options[1] != 1){
	    // options[2]: -A
	    if(options[2] == 1){
	      write(1, p, q+1 - p);
	      int countLines = 0;
	      p = q + 1;
	      // Attempts to print out n number of lines of context
	      while((q = strchr(p, '\n')) != 0 && countLines < options[5]){
	        *q = 0;
		m += n;
	        buf[m] = '\0';
   	        p = buf;
		write(1, p, q+1 - p);
		p = q + 1;
		countLines++;
	      }
	    }
	    else{
              write(1, p, q+1 - p);
	    }
          }
          else{
           counter++;
          }
        }
      }
      else{
        if(match(pattern,p)){
          *q = '\n';
	  if(options[1] != 1){
            if(options[2] == 1){
              write(1, p, q+1 - p);
              int countLines = 0;
              p = q + 1;
              while((q = strchr(p, '\n')) != 0 && countLines < options[5]){
                *q = 0;
                m += n;
                buf[m] = '\0';
                p = buf;
                write(1, p, q+1 - p);
                p = q + 1;
                countLines++;
              }
            }
            else{
              write(1, p, q+1 - p);
            }
          }
	  else{
	   counter++;
	  }
        }
      }
      p = q+1;
    }
    
    if(p == buf)
      m = 0;
    if(m > 0){
      m -= p - buf;
      memmove(buf, p, m);
    }
  }
  // currently prints once for each file instead of a total count
  if(options[1] == 1 && counter < 10){

    char* count = (char*) malloc(sizeof(char) * 5);
    count[0] = (char) counter + '0';
    count[1] = '\n';
    write(1, count, 2);
  }

}

int
main(int argc, char *argv[])
{
  int fd, i;
  int options[8];
  char *pattern;
  if(argc <= 1){
    printf(2, "usage: grep pattern [file ...]\n");
    exit();
  }
  
   pattern = argv[1];
  
  if(argc <= 2){
    grep(pattern, 0, NULL);
    exit();
  }
  // TODO: Give error if they don't provide 2nd arg for -ABC
  int position = 1;
  // Checks for command line arguments
  for(i = 1; i < argc; i++){
    if(strcmp(argv[i], "-i") == 0){
      position++;
      options[0] = 1;
    }
    if(strcmp(argv[i], "-c") == 0){
      position++;
      options[1] = 1;
    }
    if(strcmp(argv[i], "-A") == 0){
      position += 2;
      options[2] = 1;
      options[5] = (int) argv[i + 1] - '0';
      i++;
    }
    if(strcmp(argv[i], "-B") == 0){
      position += 2;
      options[3] = 1;
      options[6] = (int) argv[i + 1] - '0';
      i++;
    }
    if(strcmp(argv[i], "-C") == 0){
      position += 2;
      options[4] = 1;
      options[7] = (int) argv[i + 1] - '0';
      i++;
    }

  }
  // Disables -ABC flags if -C is set as -c trumps them
  if(options[1] == 1){
    options[2] = 0;
    options[3] = 0;
    options[4] = 0;
  }
  pattern = argv[position];
  for(i = position + 1; i < argc; i++){
        if((fd = open(argv[i], 0)) < 0){
          printf(1, "grep: cannot open %s\n", argv[i]);
          exit();
        }
        grep(pattern, fd, options);
        close(fd);
 
  }
  exit();
}

// Regexp matcher from Kernighan & Pike,
// The Practice of Programming, Chapter 9.

int matchhere(char*, char*);
int matchstar(int, char*, char*);

int
match(char *re, char *text)
{
  if(re[0] == '^')
    return matchhere(re+1, text);
  do{  // must look at empty string
    if(matchhere(re, text))
      return 1;
  }while(*text++ != '\0');
  return 0;
}

// matchhere: search for re at beginning of text
int matchhere(char *re, char *text)
{
  if(re[0] == '\0')
    return 1;
  if(re[1] == '*')
    return matchstar(re[0], re+2, text);
  if(re[0] == '$' && re[1] == '\0')
    return *text == '\0';
  if(*text!='\0' && (re[0]=='.' || re[0]==*text))
    return matchhere(re+1, text+1);
  return 0;
}

// matchstar: search for c*re at beginning of text
int matchstar(int c, char *re, char *text)
{
  do{  // a * matches zero or more instances
    if(matchhere(re, text))
      return 1;
  }while(*text!='\0' && (*text++==c || c=='.'));
  return 0;
}

