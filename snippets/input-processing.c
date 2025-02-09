char *lsh_read_line(void)
{
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (buffer == NULL) {
     fprintf(stderr, "lsh: allocation failed");
     exit(EXIT_FAILURE); // terminates the program with a failure status 1
  }

  while(1) {
    // Read a character
    c = getchar();

    // if reached EOF, replace it with a null character and return
    if (c == EOF || c == '\n') {
      buffer[position] = '\0'; // buffer[position] = * (buffer + position)
      return buffer;
    }

    else {
      buffer[position] = c;
    }
    position++;

    // if exceeded the buffer, reallocate another block
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}
