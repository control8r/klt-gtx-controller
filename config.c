int config_line_display(char* line){
	if(!strlen(line)){
		return 0;
	}

	char* token = strtok(line, " ");

	if(!strcmp(token, "lines")){
		token = strtok(NULL, " ");
		if(token){
			config.lines = strtoul(token, NULL, 10);
			return 0;
		}
	}
	else if(!strcmp(token, "address")){
		token = strtok(NULL, " ");
		if(token){
			config.display = strtoul(token, NULL, 10);
			return 0;
		}
	}
	else if(!strcmp(token, "destination")){
		token = strtok(NULL, " ");
		if(token && !strcmp(token, "network")){
			char* host = strtok(NULL, " ");
			char* port = strtok(NULL, " ");

			if(host){
				config.display_fd = network_fd(host, port ? port : DISPLAY_DEFAULT_PORT, SOCK_STREAM, 0);
				return 0;
			}
		}
		else if(token && !strcmp(token, "device")){
			token = strtok(NULL, " ");
			if(token){
				config.display_fd = open(token, O_NONBLOCK);
				return 0;
			}
		}
		printf("Unknown display connection type %s\n", token ? token : "NONE");
		return 1;
	}

	printf("Unknown/invalid display configuration option %s\n", token);
	return 1;
}

int config_line_artnet(char* line){
	if(!strlen(line)){
		return 0;
	}

	char* token = strtok(line, " ");

	if(!strcmp(token, "net")){
		token = strtok(NULL, " ");
		if(token){
			config.net = strtoul(token, NULL, 10);
			return 0;
		}
	}
	else if(!strcmp(token, "universe")){
		token = strtok(NULL, " ");
		if(token){
			config.universe = strtoul(token, NULL, 10);
			return 0;
		}
	}
	else if(!strcmp(token, "address")){
		token = strtok(NULL, " ");
		if(token){
			config.address = strtoul(token, NULL, 10);
			return 0;
		}
	}
	else if(!strcmp(token, "listen")){
		if(config.artnet_fd >= 0){
			printf("ArtNet listener already created\n");
			return 1;
		}

		char* host = strtok(NULL, " ");
		char* port = strtok(NULL, " ");

		if(host){
			config.artnet_fd = network_fd(host, port ? port : ARTNET_DEFAULT_PORT, SOCK_DGRAM, 1);
			return 0;
		}
	}

	printf("Unknown/invalid artnet configuration option %s\n", token);
	return 1;
}

int config_line_message(char* line){
	//TODO
	return 0;
}

int config_line(char* line){
	if(!strcmp(line, "[display]")){
		config.parser_state = cfg_display;
		return 0;
	}
	else if(!strcmp(line, "[artnet]")){
		config.parser_state = cfg_artnet;
		return 0;
	}
	else if(!strncmp(line, "[message ", 9)){
		//TODO parse message header
		config.parser_state = cfg_message;
		return 0;
	}

	switch(config.parser_state){
		case cfg_display:
			return config_line_display(line);
		case cfg_artnet:
			return config_line_artnet(line);
		case cfg_message:
			return config_line_message(line);
		default:
			break;
	}

	printf("Unknown/invalid configuration stanza \"%s\"\n", line);
	return 1;
}

int config_parse(char* config_path){
	int rv = 0;
	ssize_t line_length = 0, line_end;
	size_t line_alloc = 0;
	char* line_raw = NULL, *line = NULL;
	FILE* config_file = fopen(config_path, "r");
	if(!config_file){
		printf("Failed to open configuration file %s\n", config_path);
		return 1;
	}

	for(line_length = getline(&line_raw, &line_alloc, config_file); line_length >= 0; line_length = getline(&line_raw, &line_alloc, config_file)){
		//left trim
		for(line = line_raw; isspace(*line); line++){
		}

		//right trim
		for(line_end = strlen(line); line_end >= 0 && !isgraph(line[line_end]); line_end--){
			line[line_end] = 0;
		}

		if(config_line(line)){
			rv = 1;
			break;
		}
	}

	fclose(config_file);
	free(line_raw);

	if(!rv
			&& config.artnet_fd >= 0
			&& config.display_fd >= 0){
		return 0;
	}
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
