#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/param.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include "controller.h"

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
	uint8_t width;

	/* ArtNet config */
	uint8_t net;
	uint8_t universe;
	uint16_t address;

	/* Messages */
	message* message[256];
	message* last_message;
} config = {
	.artnet_fd = -1,
	.display_fd = -1,
	.shutdown = 0,
	.parser_state = cfg_none
};

int network_fd(char* host, char* port, int socktype, uint8_t listener){
	int fd = -1, status, yes = 1;
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = socktype,
		.ai_flags = (listener ? AI_PASSIVE : 0)
	};
	struct addrinfo *info, *addr_it;

	status = getaddrinfo(host, port, &hints, &info);
	if(status){
		printf("Failed to parse address %s port %s: %s\n", host, port, gai_strerror(status));
		return -1;
	}

	//traverse the result list
	for(addr_it = info; addr_it; addr_it = addr_it->ai_next){
		fd = socket(addr_it->ai_family, addr_it->ai_socktype, addr_it->ai_protocol);
		if(fd < 0){
			continue;
		}

		//set required socket options
		yes = 1;
		if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&yes, sizeof(yes)) < 0){
			printf("Failed to enable SO_REUSEADDR on socket: %s\n", strerror(errno));
		}

		if(listener){
			status = bind(fd, addr_it->ai_addr, addr_it->ai_addrlen);
			if(status < 0){
				close(fd);
				continue;
			}
		}
		else{
			status = connect(fd, addr_it->ai_addr, addr_it->ai_addrlen);
			if(status < 0){
				close(fd);
				continue;
			}
		}

		break;
	}
	freeaddrinfo(info);

	if(!addr_it){
		printf("Failed to create socket for %s port %s\n", host, port);
		return -1;
	}

	//set nonblocking
	status = fcntl(fd, F_GETFL, 0);
	if(fcntl(fd, F_SETFL, status | O_NONBLOCK) < 0){
		printf("Failed to set socket nonblocking: %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}

uint8_t calc_checksum(uint8_t* data, size_t bytes){
	uint8_t checksum = 0;
	size_t u = 0;

	for(u = 0; u < bytes; u++){
		checksum ^= data[u];
	}

	return checksum;
}

int tx_message(message* msg, uint8_t tempo, uint8_t function){
	size_t page;
	tempo /= (255 / 8);
	function >>= 4;

	tx_header hdr = {
		.lines = config.lines,
		.destination = config.display,
		.eoh = 0x03,
		.flags = HFLAGS_DEFAULT | HFLAG_CONTINUE
	};

	tx_page_hdr page_hdr = {
		.tempo = PFLAGS_TEMPO_DEFAULT | (tempo + 1),
		.function = PFLAGS_FUNCTION_DEFAULT | function,
		.flags = PFLAGS_DEFAULT
	};

	char termination[2] = {0x04, 0x04};

	for(page = 0; page < msg->pages; page++){
		if(!msg->text[page]){
			break;
		}

		if(page == msg->pages - 1
				|| !msg->text[page + 1]){
			hdr.flags &= ~HFLAG_CONTINUE;
		}

		page_hdr.index[0] = '0' + (page + 1) / 100;
		page_hdr.index[1] = '0' + ((page + 1) % 100) / 10;
		page_hdr.index[2] = '0' + (page + 1) % 10;

		termination[1] = 0x04;
		termination[1] ^= calc_checksum((uint8_t*) &hdr, sizeof(hdr));
		termination[1] ^= calc_checksum((uint8_t*) &page_hdr, sizeof(page_hdr));
		termination[1] ^= calc_checksum((uint8_t*) msg->text[page], strlen(msg->text[page]));

		printf("Transmitting page %zu of %zu, %zu bytes, Tempo %d (%d secs), Function %d\n", page + 1, msg->pages, msg->text[page] ? strlen(msg->text[page]) : 0, tempo, page_interval[tempo], function);
		printf("Page index %c%c%c, Continue %s, Checksum %02X\n", page_hdr.index[0], page_hdr.index[1], page_hdr.index[2], hdr.flags & HFLAG_CONTINUE ? "yes" : "no", termination[1]);
		printf("Contents: %s\n", msg->text[page] ? msg->text[page] : "NULL");

		write(config.display_fd, &hdr, sizeof(hdr));
		write(config.display_fd, &page_hdr, sizeof(page_hdr));
		write(config.display_fd, msg->text[page], strlen(msg->text[page]));
		write(config.display_fd, termination, 2);
	}
	return 0;
}

int artnet_process(){
	char* recv_buffer[8192];
	artnet_pkt* frame = (artnet_pkt*) recv_buffer;

	ssize_t bytes_read = read(config.artnet_fd, recv_buffer, sizeof(recv_buffer));
	if(bytes_read <= 0){
		printf("Error receiving ArtNet: %s\n", strerror(errno));
		return 1;
	}

	if(bytes_read > 20
			&& !memcmp(frame->magic, "Art-Net\0", 8)
			&& be16toh(frame->opcode) == ARTNET_OPCODE_DMX
			&& frame->universe == config.universe
			&& frame->net == config.net
			&& be16toh(frame->length) >= config.address + ARTNET_CHANNELS){
		if(!config.last_message){
			printf("ArtNet input data, no message transmission yet\n");
		}

		if(config.message[frame->data[config.address]]
				&& config.message[frame->data[config.address]] != config.last_message){
			config.last_message = config.message[frame->data[config.address]];
			return tx_message(config.last_message, frame->data[config.address + 1], frame->data[config.address + 2]);
		}
	}

	return 0;
}

int display_process(){
	char* recv_buffer[8192];

	//read and ignore bytes from the display for now
	ssize_t bytes = read(config.display_fd, recv_buffer, sizeof(recv_buffer));
	if(bytes <= 0){
		printf("Error receiving from display: %s\n", strerror(errno));
		return 1;
	}

	printf("%zu bytes from display\n", bytes);
	return 0;
}

//use the same TLU, not that pretty but avoids having to pre-declare the config struct
#include "config.c"

void usage(char* fn){
	printf("\nKLT ArtNet controller\n");
	printf("Usage:\n");
	printf("\t%s <config>\n", fn);
}

static void signal_handler(int signum){
	config.shutdown = 1;
}

int main(int argc, char** argv){
	fd_set read_fds;
	int status;

	if(argc < 2){
		printf("No configuration given\n");
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
	config.last_message = NULL;

	while(!config.shutdown){
		FD_ZERO(&read_fds);
		FD_SET(config.artnet_fd, &read_fds);
		FD_SET(config.display_fd, &read_fds);

		status = select((MAX(config.artnet_fd, config.display_fd)) + 1, &read_fds, NULL, NULL, NULL);
		if(status > 0){
			if(FD_ISSET(config.artnet_fd, &read_fds)){
				if(artnet_process()){
					break;
				}
			}

			if(FD_ISSET(config.display_fd, &read_fds)){
				if(display_process()){
					break;
				}
			}
		}
		else if(status < 0){
			printf("Input multiplexing failed: %s\n", strerror(errno));
			break;
		}
	}

	config_free();
	return EXIT_SUCCESS;
}
