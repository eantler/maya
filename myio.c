#define _GNU_SOURCE
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#define KB (1024)
#define MB (1024*KB)
#define FILE_SIZE (128*MB)

static char buf[1024 * 1024] __attribute__ ((__aligned__(4096)));

int writeRandomDataToFile(int fdInput);
void writeRandomDataToBuffer();
int writeDataInRandomOffsetsToFile(char *inputPath, int flag, size_t writeSize);

int main(int argc, char *argv[]) {

	// Assert there are 3 arguments
	if(argc != 4) {
		printf("Error: Invalid number of arguments\n");
		return 0;
	}

	// Command line's arguments
	char *inputPath;
	int writeSize;
	int flag;

	// Program's variables
	int fdInput;
	struct stat statBuf;
	int statResult;
	int isNeedToWrite;

	// Fill program variables with command line arguments
	inputPath = argv[1];
	flag = atoi(argv[2]);
	writeSize = atoi(argv[3]); // in KB!
	
	// Reset flag
	isNeedToWrite = 0;

	// Get file's metadata
	statResult = lstat(inputPath, &statBuf);

	// File does not exist or error
	if (statResult == -1) {

		// File does not exist (create it)
		if (errno == ENOENT) {

			// Create file
			fdInput = creat(inputPath, 0777);
			if (fdInput < 0) {
				printf("Error creating input file: %s\n", strerror(errno));
				return errno;
			}
			
			// Change flag
			isNeedToWrite = 1;
		}

		// Another error (exit program)
		else {
			printf("Error getting inputFile's metadata: %s\n", strerror(errno));
			return errno;
		}

	}

	// File exist and is not block device
	else if ((statBuf.st_mode & S_IFMT) != S_IFBLK) {

		// File is symbolic link (exit program)
		if ((statBuf.st_mode & S_IFMT) == S_IFLNK) {
			printf("Error: File is symbolic link\n");
			return 0;
		}

		// File has additional hard links (exit program)
		if (statBuf.st_nlink > 1) {
			printf("Error: File has additional hard links\n");
			return 0;
		}

		// File's size is not exacly 128MB
		if (statBuf.st_size != FILE_SIZE) {

			// Open and trunc it
			fdInput = open(inputPath, O_RDWR | O_TRUNC, 0777);
			if (fdInput < 0) {
				printf("Error opening input file: %s\n", strerror(errno));
				return errno;
			}
			// Update flag
			isNeedToWrite = 1;
		}
	}
	
	if(isNeedToWrite) {
	  
		// Write random data to it until it has 128MB of data
		if (!writeRandomDataToFile(fdInput))
				return errno;

		// Close the file
		close(fdInput);
	}

	// Fill buffer with random data
	writeRandomDataToBuffer();

	// Write data to file in random offsets
	if(!writeDataInRandomOffsetsToFile(inputPath, flag, writeSize*KB))
		return errno;
	else
		return 1;

}

/* int writeRandomDataToFile(int fdInput, int size)
 * Return: (0) on failure, (1) else
 * Precondition: Sizes are in bytes
 */
int writeRandomDataToFile(int fdInput) {

	int i;

	for (i = 0 ; i < (FILE_SIZE / MB) ; i++) {

		// Fill buf with  1MB of random data
		writeRandomDataToBuffer();

		// Write 1MB random data to file
		if (write(fdInput, buf, MB) < 0) {
			printf("Error writing random data to file: %s\n", strerror(errno));
			return 0;
		}
	}

	return 1;
}

/* int writeRandomDataToBuffer(char *buf, int size)
 * Precondition: Sizes are in bytes
 */
void writeRandomDataToBuffer() {
	int i;
	for (i = 0; i < MB; i++)
		buf[i] = random();
}

/* int writeDataInRandomOffsetsToFile(char *buf, int fd, int writeSize)
 * Return: (0) on failure, (1) else
 * Precondition: Sizes are in bytes
 */
int writeDataInRandomOffsetsToFile(char *inputPath, int flag, size_t writeSize) {

	// Time measurement's variables
	struct timeval start, end;
	long mtime, seconds, useconds;
	double throughput;

	// Variables
	int fdInput;
	long i, offset;
	long numberOfWrites = FILE_SIZE / writeSize;

	// Start time measurement
	gettimeofday(&start, NULL);

	// Open input file
	if (flag)
		fdInput = open(inputPath, O_RDWR | O_DIRECT, 0777);
	else
		fdInput = open(inputPath, O_RDWR, 0777);
	if (fdInput < 0) {
		printf("Error opening input file: %s\n", strerror(errno));
		return 0;
	}

	// Write random data into file numberOfWrites times
	for (i = 0; i < numberOfWrites; i++) {

		// Get random aligned offset
		offset = (random() % numberOfWrites) * writeSize;

		// Seek to random offset
		if (lseek(fdInput, offset, SEEK_SET) < 0) {
			printf("Error seeking to random offset: %s\n", strerror(errno));
			return 0;
		}

		// Write data to file
		if (write(fdInput, buf, writeSize) < 0) {
			printf("Error writing data to file in random offset: %s\n",
					strerror(errno));
			return 0;
		}
	}

	// Close inputFile
	close(fdInput);

	// End time measurement
	gettimeofday(&end, NULL);

	// Get time measurement result
	seconds = end.tv_sec - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;
	mtime = ((seconds) * 1000 + useconds / 1000.0) + 0.5;

	// Calculate run throughput
	throughput = ((128 * 1000) / (double) mtime);

	// Print run throughput
	printf("MB/sec: %f\n", throughput);

	return 1;
}
