#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include <getopt.h>
// --------------------------
typedef struct {
    const char *url;
    long start;
    long end;
    FILE *fp;
} DownloadArgs; 

typedef struct {
  char *url;        
  char *filename;   
  int max_threads; 
} DLSettings;       // settings for downloader

DLSettings settings;   
// --------------------------
#define DEFAULT_MAX_THREADS 2
// --------------------------

void parse_args(int argc, char *argv[]);


int main(int argc, char *argv[]) {
    parse_args(argc, argv);
    printf(settings.filename);
    curl_off_t res = cal_total_size();
    printf("Total size: %lld\n", (long long)res);
  
    return 0;
}

void parse_args(int argc, char *argv[]) {
  // ./downloader -u <url> -o <filename> -n <max_threads>
  int opt;
  while ((opt = getopt(argc, argv, "u:o:n:")) != -1) {
    switch (opt) {
      case 'u':
        settings.url = optarg;
        break;
      case 'o':
        settings.filename = optarg;
        break;
      case 'n':
        if (atoi(optarg) == 0) {
          fprintf(stderr, "Error: max_threads must be a number\n");
          exit(EXIT_FAILURE);
        }
        if (atoi(optarg) < 1 || atoi(optarg) > 32) {
          fprintf(stderr, "Error: max_threads must be between 1 and 32\n");
          exit(EXIT_FAILURE);
        }
        // settings.max_threads = atoi(optarg);
        // if you want to use number of user threads 
        settings.max_threads = DEFAULT_MAX_THREADS;
        break;
      default:
        fprintf(stderr, "Usage: %s -u <url> -o <filename> -n <max_threads>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  
  if (settings.url == NULL) {
    fprintf(stderr, "Usage: %s -u <url> -o <filename> -n <max_threads>\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  
  if (settings.filename == NULL) {
    fprintf(stderr, "Usage: %s -u <url> -o <filename> -n <max_threads>\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  
  if (settings.max_threads == 0) {
    settings.max_threads = DEFAULT_MAX_THREADS;
  }
}
