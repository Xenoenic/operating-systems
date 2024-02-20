#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <semaphore.h>

sem_t running;
sem_t even;
sem_t odd;

void logStart(char *tID); //function to log that a new thread is started
void logFinish(char *tID); //function to log that a thread has finished its time

void startClock(); //function to start program clock
long getCurrentTime(); //function to check current time since clock was started
time_t programClock; //the global timer/clock for the program
int lTime; //the latest start time for a thread

typedef struct thread //represents a single thread, you can add more members if required
{
	char tid[4]; //id of the thread as read from file
	unsigned int startTime;
	int state;
	pthread_t handle;
	int retVal;
} Thread;

int threadsLeft(Thread *threads, int threadCount);
int threadToStart(Thread *threads, int threadCount);
int lsTime(Thread *threads, int threadCount);
void* threadRun(void *t); //the thread function, the code executed by each thread
int readFile(char *fileName, Thread **threads); //function to read the file content and build array of threads

int main(int argc, char *argv[]) {

	Thread *threads = NULL;
	int threadCount = readFile(argv[1], &threads);

	lTime = lsTime(threads, threadCount);

	sem_init(&running, 0, 1);

	if (threads[0].tid[2] - '0' % 2 == 0 || threads[0].tid[2] - '0' == 0) {
		sem_init(&even, 0, 1);
		sem_init(&odd, 0, 0);
	} else {
		sem_init(&even, 0, 0);
		sem_init(&odd, 0, 1);
	}

	startClock();
	while (threadsLeft(threads, threadCount) > 0) {
		int i = 0;
		while ((i = threadToStart(threads, threadCount)) > -1) {
			threads[i].state = 1;
			threads[i].retVal = pthread_create(&(threads[i].handle), NULL,
					threadRun, &threads[i]);
		}
	}
	sem_destroy(&running);
	sem_destroy(&even);
	sem_destroy(&odd);
	return 0;
}

int readFile(char *fileName, Thread **threads) {
	FILE *in = fopen("sample3_in.txt", "r");
	if (!in) {
		printf(
				"Child A: Error in opening input file...exiting with error code -1\n");
		return -1;
	}

	struct stat st;
	fstat(fileno(in), &st);
	char *fileContent = (char*) malloc(((int) st.st_size + 1) * sizeof(char));
	fileContent[0] = '\0';
	while (!feof(in)) {
		char line[100];
		if (fgets(line, 100, in) != NULL) {
			strncat(fileContent, line, strlen(line));
		}
	}
	fclose(in);

	char *command = NULL;
	int threadCount = 0;
	char *fileCopy = (char*) malloc((strlen(fileContent) + 1) * sizeof(char));
	strcpy(fileCopy, fileContent);
	command = strtok(fileCopy, "\r\n");
	while (command != NULL) {
		threadCount++;
		command = strtok(NULL, "\r\n");
	}
	*threads = (Thread*) malloc(sizeof(Thread) * threadCount);

	char *lines[threadCount];
	command = NULL;
	int i = 0;
	command = strtok(fileContent, "\r\n");
	while (command != NULL) {
		lines[i] = malloc(sizeof(command) * sizeof(char));
		strcpy(lines[i], command);
		i++;
		command = strtok(NULL, "\r\n");
	}

	for (int k = 0; k < threadCount; k++) {
		char *token = NULL;
		int j = 0;
		token = strtok(lines[k], ";");
		while (token != NULL) {
			(*threads)[k].state = 0;
			if (j == 0)
				strcpy((*threads)[k].tid, token);
			if (j == 1)
				(*threads)[k].startTime = atoi(token);
			j++;
			token = strtok(NULL, ";");
		}
	}
	return threadCount;
}

void logStart(char *tID) {
	printf("[%ld] New Thread with ID %s is started.\n", getCurrentTime(), tID);
}

void logFinish(char *tID) {
	printf("[%ld] Thread with ID %s is finished.\n", getCurrentTime(), tID);
}

int threadsLeft(Thread *threads, int threadCount) {
	int remainingThreads = 0;
	for (int k = 0; k < threadCount; k++) {
		if (threads[k].state > -1)
			remainingThreads++;
	}
	return remainingThreads;
}

int threadToStart(Thread *threads, int threadCount) {
	for (int k = 0; k < threadCount; k++) {
		if (threads[k].state == 0 && threads[k].startTime == getCurrentTime())
			return k;
	}
	return -1;
}

int lsTime(Thread *threads, int threadCount) {
	int time = 0;
	for (int k = 0; k < threadCount; k++) {
		if (threads[k].startTime > time)
			time = threads[k].startTime;
	}
	return time;
}

void* threadRun(void *t) {
	logStart(((Thread*) t)->tid);

	if (getCurrentTime() < lTime) {
		if ((((Thread*) t)->tid[2] - '0') % 2 == 0) {
			sem_wait(&even);
		} else {
			sem_wait(&odd);
		}
	}
	sem_wait(&running);

	printf("[%ld] Thread %s is in its critical section\n", getCurrentTime(),
			((Thread*) t)->tid);

	sem_post(&running);

	if ((((Thread*) t)->tid[2] - '0') % 2 == 0) {
		if (getCurrentTime() >= lTime)
			sem_post(&even);
		sem_post(&odd);
	} else {
		if (getCurrentTime() >= lTime)
			sem_post(&odd);
		sem_post(&even);
	}

	logFinish(((Thread*) t)->tid);
	((Thread*) t)->state = -1;
	pthread_exit(0);
}

void startClock() {
	programClock = time(NULL);
}

long getCurrentTime() {
	time_t now;
	now = time(NULL);
	return now - programClock;
}

//done