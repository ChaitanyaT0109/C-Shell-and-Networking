#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int
main(int argc, char *argv[])
{
  int initial_count, final_count;
  int fd;
  char buffer[100];
  int bytes_read;

  // Get initial read count
  initial_count = getreadcount();
  printf("Initial read count: %d\n", initial_count);

  // Create a test file with some content
  fd = open("testfile.txt", O_CREATE | O_WRONLY);
  if(fd < 0) {
    printf("Error: could not create test file\n");
    exit(1);
  }
  
  // Write 100 bytes to the file
  char test_data[100];
  for(int i = 0; i < 100; i++) {
    test_data[i] = 'A' + (i % 26);  // Fill with repeating alphabet
  }
  
  if(write(fd, test_data, 100) != 100) {
    printf("Error: could not write to test file\n");
    close(fd);
    exit(1);
  }
  close(fd);

  // Now read 100 bytes from the file
  fd = open("testfile.txt", O_RDONLY);
  if(fd < 0) {
    printf("Error: could not open test file for reading\n");
    exit(1);
  }

  bytes_read = read(fd, buffer, 100);
  if(bytes_read != 100) {
    printf("Error: expected to read 100 bytes, but read %d\n", bytes_read);
    close(fd);
    exit(1);
  }
  close(fd);

  // Get final read count
  final_count = getreadcount();
  printf("Final read count: %d\n", final_count);

  // Verify the increase
  int increase = final_count - initial_count;
  printf("Increase in read count: %d\n", increase);

  if(increase >= 100) {
    printf("SUCCESS: Read count increased by at least 100 bytes\n");
  } else {
    printf("ERROR: Read count only increased by %d bytes (expected at least 100)\n", increase);
  }

  // Clean up
  unlink("testfile.txt");
  
  exit(0);
}