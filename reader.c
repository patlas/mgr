#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/select.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>
#include <syslog.h>
#include "reader.h"

//#define DEBUG
/* ---------------------- PRIVATE SECTION --------------------------- */

static info_t info_struct;
static pthread_t read_thread, commander_thread, watchdog_thread;
static const char *log_file = "/root/a4wp/a4wp-simulators/Bluetooth/log_file.txt";
static const char *log_file_all = "/root/a4wp/a4wp-simulators/Bluetooth/log_file_all.txt";

static uint8_t buffer[CIRCULAR_BUFFER_SIZE];

static const uint8_t *CMD[] = {
  "START_LOG",
  "CLEAR",  // start logging data at the beggining of the buffer, command only for load_detection purpose!
  "STOP",   // save logged data at the beggining of the file (overwrite it), command only for load_detection purpose!
  "SAVE",   // dump logged data from buffer into file (circular mode)
  "RESET",
  "EXIT"
};

volatile uint32_t write_size = 0;

static pthread_mutexattr_t mattr;
static pthread_mutex_t locker, slocker;
volatile bool interrupt_uart_reading = false;


static void *read_from_uart(void *args);
static void *watchdog(void *args);

volatile uint32_t dat_index = 0;
static volatile uint8_t fd_log_index = 0;

static void interrupt_read(void)
{
  pthread_mutex_lock(&locker);
  interrupt_uart_reading = true;
  pthread_mutex_unlock(&locker);
}


static bool check_interrupt(void)
{
  if(pthread_mutex_trylock(&locker) == 0)
  {
    if(interrupt_uart_reading == true)
    {
      pthread_mutex_unlock(&locker);
      return false;
    }
    else
    {
      pthread_mutex_unlock(&locker);
      return true;
    }
  }
  return true;
}

static bool check_interrupt_blocking(void)
{
  pthread_mutex_lock(&locker);

  if(interrupt_uart_reading == true)
  {
    pthread_mutex_unlock(&locker);
    return true;
  }
  else
  {
    pthread_mutex_unlock(&locker);
    return false;
  }

}



/* Starts receiving thread */
static bool start_log(info_t *s)
{
  debug_log("Received: START_LOG\n");
  if(pthread_create(&read_thread, NULL, read_from_uart, s))
  {
    interrupt_uart_reading = false;
    debug_log("Error creating uart receiver thread!\n");
    return false;
  }
  return true;
}



static bool start(info_t *s)
{
  debug_log("Received: CLEAR (LoadMonitor), reseting buffer index...\n");
  while(pthread_mutex_lock(&slocker) != 0);
  dat_index = 0;
  write_size = 0;
  debug_log("Received: CLEAR, reset done\n");
  while(pthread_mutex_unlock(&slocker) != 0);
  return true;
}

static bool reset(info_t *s)
{
  debug_log("Received: RESET, reseting buffer index...\n");
  while(pthread_mutex_lock(&slocker) != 0); 
  dat_index = 0;
  write_size = 0;
  debug_log("Received: RESET, reset done\n");
  while(pthread_mutex_unlock(&slocker) != 0);
  return true;
}


static bool save(info_t *s)
{
  int fd = 0;
  char tmp_data[FILE_BUF_SIZE];

  while(pthread_mutex_lock(&slocker) != 0);
  if(fd_log_index == FD_ALL)
  {
    debug_log("Prepare data to save into log_file_alle.txt\n");
    fd = s->fd_log_file_all;
  }
  else
  {
    debug_log("Prepare data to save into log_file.txt (LoadMonitor)\n");
    close(s->fd_log_file);
    s->fd_log_file = open(log_file, O_CREAT | O_WRONLY | O_TRUNC, 0777);
    fd = s->fd_log_file;
  } 
  
  lseek(fd,0, SEEK_SET);
  sprintf(tmp_data, "Data count: %d\n",write_size/*s->size*/);
  debug_log(tmp_data);
  if(write(fd, &buffer[0], write_size/*s->size*/) < 0)
    debug_log("Write error!\n");
  while(pthread_mutex_unlock(&slocker) != 0);


    
    
  return true;
}

/* Stops receiving thread */
static bool stop(info_t *s)
{
  debug_log("Received: STOP\n");
  fd_log_index = FD_LOAD; //set index to save data into file used by tests
  save(s);
  fd_log_index = FD_ALL; //set index to save data into file containing logs
  return true;
}

/* Stops receiving thread and terminate application */
static bool pexit(info_t *s)
{
  if(!check_interrupt_blocking())
  {
    interrupt_read(); //interrupt uart reader task and make data dump into file
  }
 
  debug_log("Received: EXIT\n");
  close(s->fd_pipe);
  close(s->fd_log_file);
  close(s->fd_log_file_all);
  ptu_debug_uart_close_device(s->fd_uart);
  err();
  exit(1);
  return true;
}

static bool (*fptr[])(info_t *s) = {
  start_log,
  start,
  stop,
  save,
  reset,
  pexit
};

/* Execute command received from named pipe -> see *CMD[] = {...} */
static int8_t execute_cmd(uint8_t *cmd)
{
  uint8_t cmd_count = (uint8_t)(sizeof(CMD)/sizeof(CMD[0]));
  uint8_t index = 0;
  
  for(index=0; index<cmd_count; index++)
  {
    if(!memcmp(CMD[index], cmd, strlen(CMD[index])))
    {
      return (int8_t)(*fptr[index])(&info_struct);
    }
  }
  debug_log("RECEIVED UNKNOWN COMMAND!\n");
  return -1;
}

/* Parse command received from pipe to avoid unnecessary end signs */ 
static bool parse_pipe_command(uint8_t *dataIN, uint8_t *dataOUT)
{
  uint8_t index = 0;
  strcpy(dataOUT, dataIN);
  return true;
}

/* Encode uart baud rate to POSIX standard */
static uint32_t encode_speed(uint32_t speed)
{
    switch(speed)
    {
        case 115200:
            return B115200;
        case 230400:
            return B230400;
        case 460800:
            return B460800;
        case 500000:
            return B500000;
        case 576000:
            return B576000;
        case 921600:
            return B921600;
        case 1000000:
            return B1000000;
        case 1152000:
            return B1152000;
        case 1500000:
            return B1500000;
        default:
            return -1;
    }
}

/* Task reading data from uart */
static void *read_from_uart(void *args)
{
  #ifdef DEBUG
    char tmp_data[FILE_BUF_SIZE];
  #endif
  int uart_fd = ((info_t*)args)->fd_uart;
  int log_fd[2] = {((info_t*)args)->fd_log_file, ((info_t*)args)->fd_log_file_all};

  int32_t d_size = 0;
  debug_log("Reader task started\n");
  while(check_interrupt())
  {
    while(pthread_mutex_lock(&slocker) !=0);
    d_size = read(uart_fd, &buffer[dat_index], CIRCULAR_BUFFER_SIZE-dat_index);
  
    
    if(d_size > 0)
    {      
      dat_index += d_size;
      write_size = dat_index;
      
      #ifdef DEBUG
        if(write_size % 1000 < 150) //Show only part of logs
        {
          sprintf(tmp_data, "Thread count: %d\n", dat_index);
          debug_log(tmp_data);
        }
      #endif 
    }    
    while(pthread_mutex_unlock(&slocker)!=0);
    usleep(5000);
  }
  debug_log("TASK INTERRUPTED - dumping dbg data into log_file_all.txt\n");
  lseek(log_fd[1],0, SEEK_SET);
  write(log_fd[1], &buffer[0], CIRCULAR_BUFFER_SIZE);
}




/* Watchdog task, close app in case of timeout caused by ATS interpreter error */
static void *watchdog(void *args)
{
  uint8_t mins = 0;
  uint16_t secs = 0;
  
  debug_log("WATCHDOG STARTED\n");
  for(mins=0; mins<WATCHDOG_TIMEOUT; mins++)
  {
    for(secs=0; secs<60; secs++)
    {
      sleep(1);
    }
  }
  debug_log("WATCHDOG TIMEOUT EXCEEDED\n");
  pexit(args);
}


static void *commander(void *args)
{
  int16_t read_data_size = 0;
  uint8_t buf[MAX_BUF_SIZE];
  uint8_t command[40];
  
  int fd_pipe = ((info_t*)args)->fd_pipe;
  
  while(1)
  {
    read_data_size = read(fd_pipe, buf, MAX_BUF_SIZE);

    if(read_data_size > 0)
    {
      if(!parse_pipe_command(buf, command))
      {
        debug_log("Error parsing pipe command!\n");
        continue;
      }

      switch(execute_cmd(command))
      {
        case 0:
          debug_log("Execution command error!\n");
          break;
        
        case 1:
          debug_log("Command executed\n");
          break;
          
        default:
          debug_log("Unknown command!\n");
          break;
      }      
    }

  }
}


/* ------------------------------------------------------------------ */


/* ---------------------- PUBLIC SECTION ---------------------------- */

/* Opens uart device making it able to received data */
int ptu_debug_uart_open_device(char *device_name)
{
    int result;

    result = open(device_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if(result < 0)
    {
        debug_log("Error: open()\n");
        err();
        return -1;
    }
    else
    {
        fcntl(result, F_SETFL, FNDELAY);
    }
    return result;
}

/* Close uart device */
int ptu_debug_uart_close_device(int interface_fd)
{
    if(close(interface_fd) < 0)
    {
        debug_log("Error: close()\n");
        err();
        return -1;
    }
    return 0;
}

/* Sets proper parameters do ensure right data transfer condition */
int ptu_debug_uart_set_attributes(int interface_fd, uint32_t speed)
{
    uint32_t speed_code;
    struct termios tty;
    speed_code = encode_speed(speed);

    if(speed_code < 0)
    {
        debug_log("SPEED ERROR!!! \n");
        return -1;
    }
    info_struct.baud_rate = speed_code;

    memset(&tty, 0, sizeof(tty));
    if(tcgetattr(interface_fd, &tty) != 0)
    {
        debug_log("Error: tcgetattr()\n");
        err();
        return -2;
    }

    cfsetospeed(&tty, (speed_t)speed_code);
    cfsetispeed(&tty, (speed_t)speed_code);

    tty.c_oflag = 0;
    tty.c_cflag |= CREAD|CLOCAL;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_lflag &= ~(ICANON | ECHO |ECHOE | ISIG);
    tty.c_iflag &= ~INPCK;
    tty.c_iflag |= IGNPAR;
    tty.c_iflag &= ~IXOFF;
    tty.c_iflag |= IGNBRK;
    tty.c_iflag |= IGNCR;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;


    if(tcsetattr(interface_fd, TCSANOW, &tty) != 0)
    {
        debug_log("Error: tcsetattr()\n");
        err();
        return -3;
    }

}


void debug_log(char *s)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char tstr[22];
  sprintf(tstr, "%04d-%02d-%02d %02d:%02d:%02d : ", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  
  FILE *fp = fopen("/root/a4wp/a4wp-simulators/dbg_logger/dbg_pipe", "w");
  fprintf(fp, DEBUG_TAG);
  fprintf(fp, "%s", tstr);
  fprintf(fp, "%s", s);
  fflush(fp);
  fclose(fp);

  openlog("ATS-SIM", LOG_NDELAY, LOG_USER);
  syslog(LOG_DEBUG,DEBUG_TAG "%s", s);
  closelog();
}

/* Arguments: dev_name, baudrate, fifo_pipe (file name) */ 
int main(int argc, char **argv)
{
  
  uint8_t fifo_name[60];
  strcpy(&fifo_name[0], argv[3]);
  uint8_t *dev_name = argv[1];
  uint32_t baudrate = atol(argv[2]);
  
  int fd_pipe;
  
  memcpy(info_struct.dev_name, dev_name, sizeof(dev_name));
  
  if(pthread_mutex_init(&locker, NULL) != 0)
  {
    debug_log("Error initing mutex!\n");
    return -1;
  }
  
  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
  
  if(pthread_mutex_init(&slocker, &mattr) != 0)
  {
    debug_log("Error initing mutex!\n");
    return -1;
  }
  

  fd_pipe = open(fifo_name, O_RDONLY | O_NONBLOCK);
  if(fd_pipe < 0)
  {
     debug_log("Error opening fifo \n");
     return -1;
  }
  info_struct.fd_pipe = fd_pipe;
  
  int fd_uart = ptu_debug_uart_open_device(dev_name);
  if(fd_uart < 0)
  {
    debug_log("Error opening uart device \n");
    return -2;
  }
  info_struct.fd_uart = fd_uart;
  ptu_debug_uart_set_attributes(fd_uart, baudrate);

  
  int fd_log = open(log_file, O_WRONLY | O_CREAT, 0777);
  if(fd_log < 0)
  {
    debug_log("Error opening log file \n");
    return -3;
  }
  
  int fd_log_all = open(log_file_all, O_WRONLY | O_CREAT, 0777);
  if(fd_log_all < 0)
  {
    debug_log("Error opening log_all file \n");
    return -3;
  }
  
  info_struct.fd_log_file = fd_log;
  info_struct.fd_log_file_all  = fd_log_all;
  /*if(pthread_create(&watchdog_thread, NULL, watchdog, &info_struct))
  {
    debug_log("Error creating watchdog thread!\n");
    return false;
  }*/
  
  if(pthread_create(&commander_thread, NULL, commander, &info_struct))
  {
    debug_log("Error creating commander thread!\n");
    return false;
  }
  
  /* wait until commander thread exit (received EXIT command) */
  pthread_join(commander_thread, NULL); 
  
  /* unlock all mutexes */
  pthread_mutex_unlock(&locker);
  pthread_mutex_unlock(&slocker);
  
  /* free obtained resources */
  pthread_mutex_destroy(&locker);
  pthread_mutex_destroy(&slocker);

  return 1;
}
