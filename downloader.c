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
    curl_off_t content_length;
} DLSettings; // settings for downloader

typedef struct {
    curl_off_t start;  // Start of byte range
    curl_off_t end;    // End of byte range
    pthread_t thread;   // Thread identifier
    char *buffer;       // Buffer to hold downloaded data
    size_t size;        // Size of the downloaded data
} thread_info;

DLSettings settings;   
pthread_mutex_t completed_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread synchronization
int completed_counter = 0; // Counter to track completed threads
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
    printf("Filename: %s\n", settings.filename);
    cal_total_size();
    printf("Total size: %lld\n", (long long)settings.content_length);
    download_manager();
    return 0;
}

void parse_args(int argc, char *argv[]) {
    // ./downloader -u <url> -o <filename> -n <max_threads>
    int opt;
    settings.max_threads = DEFAULT_MAX_THREADS; // Default value
    while ((opt = getopt(argc, argv, "u:o:n:")) != -1) {
        switch (opt) {
            case 'u':
                settings.url = optarg;
                break;
            case 'o':
                settings.filename = optarg;
                break;
            case 'n':
                if (atoi(optarg) < 1 || atoi(optarg) > 32) {
                    fprintf(stderr, "Error: max_threads must be between 1 and 32\n");
                    exit(EXIT_FAILURE);
                }
                settings.max_threads = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -u <url> -o <filename> -n <max_threads>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (settings.url == NULL || settings.filename == NULL) {
        fprintf(stderr, "Usage: %s -u <url> -o <filename> -n <max_threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
}

void cal_total_size() {
    // Fetch content length
    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, settings.url);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_perform(curl);
        
        curl_off_t res = 0;
        curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &res);
        curl_easy_cleanup(curl);

        if (res <= 0) {
            fprintf(stderr, "ERROR | Could not fetch content length\n");
            exit(EXIT_FAILURE);
        }

        settings.content_length = res;
    }
}

void download_manager() {
    // Calculate chunk size
    curl_off_t chunk_size = settings.content_length / settings.max_threads;

    // Allocate an array of thread_info structures
    thread_info *threads_info = malloc(settings.max_threads * sizeof(thread_info));
    if (!threads_info) {
        perror("Failed to allocate thread_info");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < settings.max_threads; i++) {
        threads_info[i].start = i * chunk_size; // Start of this thread's range
        threads_info[i].end = (i == settings.max_threads - 1) ? settings.content_length - 1 : (i + 1) * chunk_size - 1; // End of this thread's range
        
        // Allocate buffer for data
        threads_info[i].size = threads_info[i].end - threads_info[i].start + 1; // Size of the downloaded data
        threads_info[i].buffer = malloc(threads_info[i].size); // Allocate buffer
        if (!threads_info[i].buffer) {
            perror("Failed to allocate buffer");
            exit(EXIT_FAILURE);
        }

        // Create the thread
        if (pthread_create(&threads_info[i].thread, NULL, worker_thread, &threads_info[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to finish using the completed counter
    for (int i = 0; i < settings.max_threads; i++) {
        pthread_join(threads_info[i].thread, NULL); // Wait for thread to finish
    }

    // Write combined data to the final file
    FILE *fp_final = fopen(settings.filename, "wb");
    if (!fp_final) {
        perror("Failed to open final output file");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < settings.max_threads; i++) {
        fwrite(threads_info[i].buffer, 1, threads_info[i].size, fp_final);
        free(threads_info[i].buffer); // Free the buffer after writing
    }

    fclose(fp_final);
    free(threads_info); // Clean up
}

void *worker_thread(void *arg) {
    // Cast argument to thread_info type
    thread_info *info = (thread_info *)arg;

    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, settings.url);
        
        // Set the range for the download
        char range_str[50];
        snprintf(range_str, sizeof(range_str), "%lld-%lld", info->start, info->end);
        curl_easy_setopt(curl, CURLOPT_RANGE, range_str);

        // Set the write callback to store downloaded data in buffer
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, info->buffer);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL); // Using default write function

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Download failed: %s\n", curl_easy_strerror(res));
        }

        // Clean up
        curl_easy_cleanup(curl);
    }

    // Increase completed counter
    pthread_mutex_lock(&completed_mutex);
    completed_counter++;
    pthread_mutex_unlock(&completed_mutex);

    return NULL;
}