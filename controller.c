#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

static struct {
	int artnet_fd;
	int display_fd;
	volatile sig_atomic_t shutdown;
	enum {
		cfg_none,
		cfg_display,
		cfg_artnet,
		cfg_message
	} parser_state;

	/* Display config */
	uint8_t lines;
	uint8_t display;

	/* ArtNet config */
	uint8_t net;
	uint8_t universe;
	uint16_t address;
} config = {
	.artnet_fd = -1,
	.display_fd = -1,
	.shutdown = 0,
	.parser_state = cfg_none
};

int config_parse(char* config_path){
	FILE* config_file = fopen(config_path, "r");
	if(!config_file){
		printf("Failed to open configuration file %s\n", config_path);
		return 1;
	}

	//TODO
	fclose(config_file);
	return 1;
}

void config_free(){
	if(config.artnet_fd >= 0){
		close(config.artnet_fd);
	}

	if(config.display_fd >= 0){
		close(config.display_fd);
	}
}

void usage(char* fn){
	printf("\nKLT ArtNet controller\n");
	printf("Usage:\n");
	printf("\t%s <config>\n", fn);
}

static void signal_handler(int signum){
	config.shutdown = 1;
}

int main(int argc, char** argv){
	if(argc < 2){
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if(config_parse(argv[1])){
		printf("Failed to parse configuration\n");

		config_free();
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	signal(SIGINT, signal_handler);

	while(!config.shutdown){
		//TODO core loop
	}

	config_free();
	return EXIT_SUCCESS;
}
