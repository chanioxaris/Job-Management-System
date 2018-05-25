#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
# include <errno.h>


int main(int argc, char* argv[]) {
	int i, fd_jms_in, fd_jms_out;
	char jms_in[20], jms_out[20], operations_file[20];
	char buf_in[512], buf_out[512];

	//Parse arguments from command line
	if (argc == 5) {
		for (i = 1 ; i < (argc-1) ; i++) {
			if (!strcmp(argv[i], "-w")) {
				stpcpy(jms_in, argv[++i]);
				continue;
			}
			if (!strcmp(argv[i], "-r")) {
				stpcpy(jms_out, argv[++i]);
				continue;
			}

			printf("Wrong input arguments! \n");
			return -1;
		}
	}
	else {
		printf("Wrong input arguments! \n");
		return -1;
	}

	//Open fifos for communication with coord
	if ((fd_jms_in = open(jms_in , O_WRONLY)) < 0) {
		perror("open");
		exit(1);
	}

	if ((fd_jms_out = open(jms_out , O_RDONLY)) < 0) {
		perror("open");
		exit(1);
	}

	printf("Enter your command: ");

	//Read commands from user via command line
	while(fgets(buf_in, sizeof(buf_in), stdin) != NULL) {
		//Send input to coord
		if (write(fd_jms_in,  buf_in, sizeof(buf_in) + 1) == -1) {
			perror("write");
			exit(2);
		}

		if (read(fd_jms_out,  buf_out, sizeof(buf_out) + 1) == -1) {
			perror("read");
			exit(3);
		}

		if (!strcmp(buf_in, "shutdown\n")) {
			close(fd_jms_in);
			close(fd_jms_out);

			exit(0);
		}

		printf("%s\n", buf_out);
		fflush(stdout);

		printf("Enter your command: ");
	}
}
