#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include <getopt.h>
// --------------------------

typedef struct {
  char *url;        
  char *filename;   
  int max_threads; 
  FILE *fp;
  curl_off_t content_length;
} DLSettings;       // settings for downloader

typedef struct {
    curl_off_t start;  // Start of byte range
    curl_off_t end;    // End of byte range
    pthread_t thread;   // Thread identifier
} thread_info;

DLSettings settings;   
// --------------------------
#define DEFAULT_MAX_THREADS 2
// --------------------------

void parse_args(int argc, char *argv[]);
void cal_total_size();
void download_manager();
void *worker_thread(void *arg);
// --------------------------

int main(int argc, char *argv[]) {
    parse_args(argc, argv);
    printf(settings.filename);
    cal_total_size();
    printf("Total size: %lld\n", (long long)settings.content_length);
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


// Open output file
    FILE *fp = fopen(settings.filename, "wb");
    if (!fp) {
        perror("Failed to open file");
       exit(EXIT_FAILURE);
    }
    fclose(fp);
}

void cal_total_size(){
  // Fetch content length
  CURL *curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, settings.url);
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
  curl_easy_perform(curl);
  curl_off_t res = 0;
  curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &res);
  curl_easy_cleanup(curl);

  if (res <= 0) {
    printf("ERROR | Could not fetch content length\n");
    exit(EXIT_FAILURE);
  }

  settings.content_length = res;

}


void download_manager() {
    // Calculate chunk size
    curl_off_t chunk_size = settings.content_length / settings.max_threads;
}

void *worker_thread(void *arg) {
   
}