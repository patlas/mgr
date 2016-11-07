#ifndef _READER_H_
#define _READER_H_

#define MAX_BUF_SIZE 1024
#define FILE_BUF_SIZE 1024
#define CIRCULAR_BUFFER_SIZE  10000000 // 10MB
#define WATCHDOG_TIMEOUT 5 //mins

//#define DEBUG

#ifdef DEBUG
  #define err() debug_log("%s\n", strerror(errno));
#else
  #define err()
#endif

#define DEBUG_TAG "[DEBUG UART] "
//#define debug_log(...) printf(DEBUG_TAG); printf(__VA_ARGS__); fflush(stdout)

#define FD_LOAD 1
#define FD_ALL  0

typedef struct {
  uint32_t baud_rate;
  int fd_pipe;
  int fd_log_file;
  int fd_log_file_all;
  int fd_uart;
  uint32_t size;
  char dev_name[20];
} info_t;


void debug_log(char *s);
int ptu_debug_uart_open_device(char *device_name);
int ptu_debug_uart_close_device(int interface_fd);
int ptu_debug_uart_set_attributes(int interface_fd, uint32_t speed);




#endif
