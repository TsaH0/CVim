#include "cvim.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#define VERSION "1.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)
enum editorKey{
  ARROW_LEFT = 1000,ARROW_RIGHT ,ARROW_UP ,ARROW_DOWN ,HOME_KEY,END_KEY,PAGE_UP,PAGE_DOWN,DEL_KEY
};
typedef struct erow{
  int size;
  char *chars;
}erow;
typedef struct editorConfigSettings{

  int coloff;
  int rowoff;
  int cx;
  int cy;
  struct termios original_terminal_state;
  int screenrows;
  int numrows;
  erow* rows;
  int screencols;
}editorConfigSettings;
struct editorConfigSettings currConfig;
void die(const char* s)
{

  perror(s);
  exit(1);
}

typedef  struct buffer{
  char* b;
  int len;
}buffer;
void disableRawMode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &currConfig.original_terminal_state);
}

void enableRawMode()
{
  tcgetattr(STDIN_FILENO, &currConfig.original_terminal_state);
  atexit(disableRawMode);
 ; 
  struct termios editor_state= currConfig.original_terminal_state;
  editor_state.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  editor_state.c_oflag &= ~(OPOST);
  editor_state.c_cflag |= (CS8);
  editor_state.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  editor_state.c_cc[VMIN] = 0;
  editor_state.c_cc[VTIME] = 100;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor_state);
   


}

void appendBuffer(buffer* ab,const char* s , int len)
{
  char * newBuffer = realloc(ab->b,ab->len+len);
  if(newBuffer==NULL) return ;
  memcpy(&newBuffer[ab->len],s,len);
  ab->b = newBuffer;
  ab->len+=len;
}
void freeBuffer(buffer* ab)
{
  free(ab->b);
}
void editorScroll()
{
  if(currConfig.cy<currConfig.rowoff)
  {
    currConfig.rowoff = currConfig.cy;
  }
  if(currConfig.cy>=currConfig.rowoff+currConfig.screenrows)
  {
    currConfig.rowoff = currConfig.cy-currConfig.screenrows+1;
  }
  if(currConfig.cx<currConfig.coloff)
  {
    currConfig.coloff = currConfig.cx;
  }
  if(currConfig.cx>=currConfig.coloff+currConfig.screencols)
  {
    currConfig.coloff = currConfig.cx-currConfig.screencols+1;
  }
}
void editorDrawRows(buffer* ab)
{
  for(int i = 0;i<currConfig.screenrows;i++)
  {
    int filerow = i+ currConfig.rowoff;
    if(i>=currConfig.numrows)
    {
    if(i==currConfig.screenrows/3 && currConfig.numrows==0)
    {
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome), "Cvim editor -- version %s",VERSION);
      if(welcomelen>currConfig.screencols)
      {
        welcomelen  = currConfig.screencols;
      }
      int padding = (currConfig.screencols-welcomelen)/2;
      if(padding)
      {
        appendBuffer(ab, "~", 1);
        padding--;
      }
      while(padding--)
      {
        appendBuffer(ab , " " , 1);
      }
        appendBuffer(ab , welcome  , welcomelen);
    }
    else{
      
  
    appendBuffer(ab,"~",1);
    }
    }
    else{
      int len = currConfig.rows[filerow].size-currConfig.coloff;
      if(len<0) len =0;
      if(len>currConfig.screencols)
      {
        len = currConfig.screencols;
      }
      appendBuffer(ab, (const char *)&currConfig.rows[filerow].chars[currConfig.coloff], len);
    }

    appendBuffer(ab,"\x1b[K",3);
    if(i<currConfig.screenrows-1)
    {

    appendBuffer(ab,"\r\n",2);
    }
  }
}
void editorRefreshScreen()
{
  buffer ab = {NULL,0};
  appendBuffer(&ab,"\x1b[?25l",6);
  appendBuffer(&ab,"\x1b[H",3);
  editorDrawRows(&ab);
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH",currConfig.cy-currConfig.rowoff+1,currConfig.cx+1);
  appendBuffer(&ab, buf, strlen(buf));

  appendBuffer(&ab,"\x1b[?25h",6);
  write(STDOUT_FILENO, ab.b, ab.len);
  freeBuffer(&ab);

}

int editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  if(c=='\x1b')
  {
    char seq[3];
    if(read(STDIN_FILENO,&seq[0],1)!=1) return '\x1b';
    if(read(STDIN_FILENO,&seq[1],1)!=1) return '\x1b';
    if(seq[0]=='[')
    {
      if(seq[1]>='0' && seq[1]<='9')
      {
        if(read(STDIN_FILENO, &seq[2], 1)!=1) return '\x1b';
        if(seq[2]=='~')
        {
          switch(seq[1])
          {
            case '1': return HOME_KEY;
            case '3': return DEL_KEY;
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
          }
        }
      }
      else{
      switch (seq[1])
      {
        case 'A': return ARROW_UP;
        case 'B': return ARROW_DOWN;
        case 'C': return ARROW_RIGHT;
        case 'D': return ARROW_LEFT;
      }
      }
    }
    return '\x1b';
  }
  else{
  return c;
  }
}
int getCursorPosition(int *rows,int *cols)
{
  char buf[32];
  unsigned int i = 0;
  if(write(STDOUT_FILENO,"\x1b[6n",4)!=4) return -1;

  while(i<sizeof(buf)-1)
  {
    if(read(STDIN_FILENO, &buf[i], 1)!=1) break;
    if(buf[i]=='R') break;
    i++;
  }
  buf[i]='\0';

  if(buf[0]!='\x1b' || buf[1]!='[') return -1;
  if(sscanf(&buf[2], "%d;%d",rows,cols)!=2) return -1 ;
  return 0;
}
int getWindowSize(int *rows,int* cols)
{
  struct winsize ws;
  if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws)==-1 || ws.ws_col==0)
  {
    if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12)!=12)
    {
      return - 1;

    }
    return getCursorPosition(rows,cols);
  }
  else{
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }

}

void editorAppendRow(char* s, size_t len)
{
  currConfig.rows= realloc(currConfig.rows, sizeof(erow)*(currConfig.numrows+1));
  int at = currConfig.numrows;
  currConfig.rows[at].size = len;
  currConfig.rows[at].chars = malloc(len+1);
  memcpy(currConfig.rows[at].chars,s,len);
  currConfig.rows[at].chars[len]='\0';
  currConfig.numrows++;
}
void editorOpen(char* filename)
{
  FILE* fp = fopen(filename , "r");
  if(!fp) die("fopen");

  char* line ;
  size_t linecap = 0;
  ssize_t linelen ;
  while((linelen = getline(&line,&linecap,fp))!=-1)
  {
    while(linelen>0 &&(line[linelen-1]=='\n' || line[linelen-1]=='\r'))
    {
      linelen--;
    }
    editorAppendRow(line,linelen);

  }
  free(line);
  fclose(fp);
}
void moveCursor(int key)
{
  erow* row = (currConfig.cy>=currConfig.numrows)?NULL:&currConfig.rows[currConfig.cy];
  switch (key) 
  {
    case ARROW_LEFT:
    if(currConfig.cx>0){
      currConfig.cx--;
      }
    
    break;
    case ARROW_RIGHT:
  
      if(row && currConfig.cx<row->size)
      {
    currConfig.cx++;
      }
    break;
    case ARROW_UP:

      if(currConfig.cy>0)
      {
        currConfig.cy--;
      }
    break;
    case ARROW_DOWN:
      if(currConfig.cy<currConfig.numrows){
    currConfig.cy++;
      }
    break;
  }
}
/*** input ***/
void editorProcessKeypress() {
  int c = editorReadKey();
  switch (c) {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
    break;
    case HOME_KEY:
    currConfig.cx = 0;
    break;
    case END_KEY:
    currConfig.cx = currConfig.screencols-1;
    break;
    case DEL_KEY:

    break;
    case PAGE_UP:
    case PAGE_DOWN:
      {
        int times = currConfig.screenrows;
        while(times--)
        {
          moveCursor(c==PAGE_UP?ARROW_UP:ARROW_DOWN);
        }

      }
    break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
    moveCursor(c);
    break;
    default:printf("%c",c);
      break;
  }
}
void initEditor()
{
  currConfig.cx = 0;
  currConfig.coloff = 0;
  currConfig.cy = 0;
  currConfig.numrows = 0;
  currConfig.rowoff = 0;
  currConfig.rows= NULL;
  if(getWindowSize(&currConfig.screenrows, &currConfig.screencols)==-1)
  {
    die("getWindowSize");
  }
}
int main(int argc,char** argv)
{
  enableRawMode();
  initEditor();
  if(argc>=2)
  {
   editorOpen(argv[1]);
  }
  while(1)
  {
    editorRefreshScreen();
    editorProcessKeypress();
    
  
   }
    return 0;
}
