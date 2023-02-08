#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAXW 1000
#define MAXCOM 100

#define TOK_BUFSIZE 64                    //se dfine buffer para los argumentos
#define TOK_DELIM " \t\r|\n\a:"             // delimitadores

#define MAXCOMMANDLIST 4

#define REDDELIM "<>"

int init_procesos(char **args);
void bg_clr(int wait);

int contadorDeProcesosEnElBackground=0; 
int contador=0;
int estado_;
int child_status; 

typedef struct{
    int flag;
    pid_t pid;
}job;

job jobs[10];

void loopShell(char** argv);
char *leerLineaCmd(void);
char **separador_linea(char *linea);
int execute(char **args);
int cantfunc();

void trimleadingandTrailing(char *s);

int funcion_cd(char **args);
int funcion_echo(char **args);
int funcion_clr(char **args);
int funcion_help(char **args);
int funcion_quit(char **args);

int executeRedirection(char* linea, char** lineas);
int contador_red(char* linea, char** lineas);

char *arraycomandos[] = {
  "cd",
  "echo",
  "clr",
  "help",
  "quit",
  "exit",
  "clear"
};

void batchFile(char **argv, int *estado);

int main(int argc, char **argv)
{
    
    int estado=0; 
    
    for (int i = 0; i < argc; i++){
        printf("argv[%d] = %s\n", i, argv[i]);
    }
    
    if (argc > 1){   
      batchFile(argv, &estado);
    }
    
    printf("El codigo de estado es: %d \n", estado);
    loopShell(argv);    
} 

void batchFile(char **argv, int *estado){
  char **args;
  FILE *fd;
  char buffer[40];

  fd = fopen(argv[1], "r"); 
  if(fd == NULL){                                    
    printf("Error en la apertura del archivo");
    exit(1);
  }
  while(!feof(fd)){

    fgets(buffer,40, fd);                            
    args = separador_linea(buffer);
    *estado = execute(args); 
  }    
  fclose(fd);
  
}


void loopShell(char **argv){
    
    char *linea;
    char **args;
    int estado;
    int redireccion = 0;
    char *lineasRed[3];
    
    char hostname[64];          
    gethostname(hostname, sizeof(hostname));

    char *username = getenv("USER");

    printf("\n");
    printf("#################################################################################################\n");
    printf("###MyShell. Introduzca el comando 'help' para ver los comandos disponibles###\n");
    printf("#################################################################################################\n\n");
 
    //Bucle de la shell
    do {
      char cwd[1024];
      getcwd(cwd,sizeof(cwd));
      printf("%s@%s:%s:~$ ",username,hostname,cwd);
      linea = leerLineaCmd();

      redireccion = contador_red(strdup(linea),lineasRed);

      if (redireccion){    //era else if no if    
        estado = executeRedirection(linea, lineasRed);       
      }
      else{
        args = separador_linea(linea);                          
        estado = execute(args);
      }

      if(contadorDeProcesosEnElBackground){
        bg_clr(0);
      }
    } while (estado); 
    bg_clr(1);

}

char *leerLineaCmd(void)
{
  int bufsize = 2048;
  
  char * buffer = malloc(sizeof(char) * bufsize);
  int characterLeido;
  for (int i=0; ; i++){
    characterLeido = getchar();

    if (characterLeido != EOF || characterLeido != '\n') {
      buffer[i] = characterLeido;
    } else {
      buffer[i] = '\0';
      return buffer;
    }
  }
}



char **separador_linea(char *linea)
{
  int bufsize = TOK_BUFSIZE; 
  int position = 0;

  
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  token = strtok(linea, TOK_DELIM);

  while (token != NULL) {
    trimleadingandTrailing(token);
    tokens[position] = token;
    printf("ASDASD dentro:%s\n",tokens[position]);
    position++;

    token = strtok(NULL,TOK_DELIM);
  }   
  tokens[position] = NULL;
  return tokens;
}

int execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < cantfunc(); i++) {
    if (strcmp(args[0], arraycomandos[i]) == 0) {
      switch (i){
        case 0:
          return funcion_cd(args);
          break;
        case 1:
          return funcion_echo(args);
          break;
        case 2:
          return funcion_clr(args);
          break;
        case 3:
          return funcion_help(args);
          break;
        case 4:
          return funcion_quit(args);
          break;
        case 5:
          return funcion_quit(args);
          break;
        case 6:
          return funcion_clr(args);
          break;
      }
    }
  }
  
  return init_procesos(args);
}

void trimleadingandTrailing(char *s)
{
    int  i,j;
 
    for(i=0;s[i]==' '||s[i]=='\t';i++);

    for(j=0;s[i];i++)
    {
        s[j++]=s[i];
    }
    s[j]='\0';
    for(i=0;s[i]!='\0';i++)
    {
        if(s[i]!=' '&& s[i]!='\t'&& s[i]!='$'&& s[i]!='\n')
                j=i;
    }
    s[j+1]='\0';
}

int cantfunc() {
  return sizeof(arraycomandos) / sizeof(char *);
}


//es para el item 3
int init_procesos(char **args){
    int i=0;
    int flag_bg=0;
    pid_t pid;//pid_t viene de linux
    
    while(args[i]!=NULL){
      i++;
    }

  if(strcmp(args[i-1],"&")==0){ 
    flag_bg=1;
    args[i-1]=NULL;
  }
  
  pid = fork ();
  
  if (pid != 0) {
    if(flag_bg){
        contador++; 
        printf("Parent PID: %d PID: %d\n", getppid(), pid);
    }          

    else{     
        estado_ = child_status;       
        waitpid(pid,&child_status,0);

        if (WIFEXITED (child_status))
            printf ("El proceso hijo finalizo normalmente, con exit code %d\n",WEXITSTATUS (child_status));
        else
            printf ("El proceso hijo finalizo de forma anormal\n");
    }
  }
  
  else {  
    char *path;
    char **paths;
  
    paths = separador_linea(getenv("PATH"));
    int i=0;

    while(paths[i]!=NULL){
      path=strdup(paths[i]); 
      strncat(path,"/",20);
      strncat(path,args[0],20);
      if(access(path, F_OK ) == 0 ) {    
        execv(path,args);                
        break;
      }
      i++; 
    }
    fprintf (stderr, "Ocurrio un error en execv\n");
    abort ();
  }  
  return 1;
}


//Funcion de Redireccion
int executeRedirection(char* linea, char** lineas){
  int flag = 0;
  int i=0;  
  char **arg0;
  char **arg1;
  char *c = linea;
  char *comando;
  char *archivo;

  pid_t p;
  while(!flag){
    if(c[i] == '<'){flag = 1;}    //se levanta el flag correspondiente segun el simbolo ingresado
    if(c[i] == '>'){flag = 2;}
    i++;
  }

  switch(flag){
    case 1:      
      
      comando = strtok(linea, "<");     //comando a ejecutar   
      archivo = strtok(NULL, "\n");     //archivo a leer
      arg1 = separador_linea(archivo);
      arg0 = separador_linea(comando);     

      p = fork();
      if (p < 0) {//error al forkear
          printf("\nCould not fork");
          return 1;
        }

      if (p == 0){  
        //Proceso Hijo       
        int file = open(arg1[0],O_RDONLY | O_CREAT, 0777);
        if(file == -1){
          return 2;
        }

        dup2(file, STDIN_FILENO); 
        close(file);
        
        char *path;
        char **paths;
        paths = separador_linea(getenv("PATH"));
        int i=0;

        while(paths[i]!=NULL){
          
          path=strdup(paths[i]);
          strncat(path,"/",20);
          strncat(path,arg0[0],20);
          
          if(access(path, F_OK ) == 0 ) {
            execv(path,arg0);
            break;
          }
          i++; 
        }
        fprintf (stderr, "Ocurrio un error en execv\n");    //ningun path conincide            
           
      }
      waitpid(p,NULL,WUNTRACED);
      break;
    case 2:      
      comando = strtok(linea, ">");
      archivo = strtok(NULL, "\n");
      arg1 = separador_linea(archivo);
      arg0 = separador_linea(comando);     

      p = fork();
      if (p < 0) {
          printf("\nCould not fork");
          return 1;
        }

      if (p == 0){   
        //Proceso hijo      
        int file = open(arg1[0],O_WRONLY | O_CREAT, 0777);  //se abre el archivo indicado como escritura
        if(file == -1){
          return 2;
        }
        dup2(file, STDOUT_FILENO);
        close(file);
        
        char *path;
        char **paths;
        paths = separador_linea(getenv("PATH"));
        int i=0;

        while(paths[i]!=NULL){
          
          path=strdup(paths[i]);
          strncat(path,"/",20);
          strncat(path,arg0[0],20);
          
                    
          if(access(path, F_OK ) == 0 ) {
            execv(path,arg0);
            break;
          }
          i++; 
        }
        fprintf (stderr, "Ocurrio un error en execv\n");
      }
      waitpid(p,NULL,WUNTRACED);      
      break;
    default:
      break;
  }    
 
  return 1;
}

//Funcion que detecta si se ingresaron simbolos de redireccion
int contador_red(char* linea, char** lineas){
  int i=0;
  for(;;){
    lineas[i] = strsep(&linea, REDDELIM);
    if(lineas[i] == NULL){
      break;
    }
    i++;
  }
  return i-1;
}


//---------------------------------------------------------
//funciones que se admiten de la terminal o cmd
//---------------------------------------------------------
//cambiar de directorio
int funcion_cd(char **args)
{

  char cwd[1024];
  getcwd(cwd,sizeof(cwd)); 

  printf("arg1: %s \n",args[0]);
  printf("arg2: %s \n",args[1]);
  printf("arg3: %s \n",args[2]);

  if (args[1] != NULL && strcmp(args[1],"")) {

    if ((strcmp(args[1],"-")==0)){
      args[1] = getenv("OLDPWD");
    }
    
    if(!chdir(args[1])){
      setenv("OLDPWD",cwd,1);
      getcwd(cwd,sizeof(cwd));
      setenv("PWD",cwd,1);
    }
    else if(chdir(args[1]) != 0){
      printf("error: el directorio <%s> no existe \n",args[1]);        
    }      

  }        
  else {
    printf("especifica un argumento \"cd\"\n");
  }  
  return 1;
}

//funcion de echo
int funcion_echo(char **args)
{
  if (args[1] == NULL) {
    printf("\nespecifica un argumento para el echo\n");
  }
  
  int j=1;
    while(args[j]!=NULL){

      if(args[j][0] == '$'){
        args[j]++;
        printf("%s ", getenv(args[j]));
      } else{
        printf("%s ",args[j]);
      }
      
      j++;    
    }

    printf("\n");
    return 1;
} 

//funcion de clear pantalla
 int funcion_clr(char **args)
{
  for(int i=0; i<1000;i++){
    printf("\033[3J\033c");
  }
  return 1;
} 

//Funcion help
int funcion_help(char **args)
{
  int i;  
  printf("Los comandos disponibles son.\n"); 
  for (i = 0; i < cantfunc(); i++) {
    printf("  %s\n", arraycomandos[i]);
  }
  return 1;
}

//Funcion quit
int funcion_quit(char **args)
{
  return 0;
}

void bg_clr(int wait){
  pid_t pid;
  int   estado;

  if(wait) 
    pid = waitpid(-1, &estado, 0);
  else
    pid = waitpid(-1, &estado, WNOHANG); 

  if(pid>0){
    int indice=0;
    contador-=1;
  }
  return;
}