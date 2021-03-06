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
	else if(!strcmp(token, "width")){
		token = strtok(NULL, " ");
		if(token){
			config.width = strtoul(token, NULL, 10);
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
	size_t page_length = 0, additional_length = 0, page;
	message* last = config.last_message;

	if(strlen(line) == 0 || last->pages == 0){
		//new page
		if(last->text && !last->text[last->pages - 1]){
			return 0;
		}
		last->text = realloc(last->text, (last->pages + 1) * sizeof(char*));
		last->text[last->pages] = NULL;
		last->pages++;
	}

	page = last->pages - 1;

	if(strlen(line) == 0){
		return 0;
	}

	page_length = last->text[page] ? strlen(last->text[page]) : 0;
	additional_length = config.width ? ((strlen(line) % config.width) ? (strlen(line) / config.width + 1) * config.width : strlen(line)) : strlen(line);

	//printf("Extending %zu page length from %d to %zu for %s\n", page, last->text[page] ? strlen(last->text[page]) : 0, page_length + additional_length, line);

	last->text[page] = realloc(last->text[page], (page_length + additional_length + 1) * sizeof(char));
	if(!last->text[page]){
		printf("Failed to allocate memory\n");
		return 1;
	}

	memset(last->text[page] + page_length, 0x20, additional_length);
	last->text[page][page_length + additional_length] = 0;
	strncpy(last->text[page] + page_length, line, strlen(line));

	//printf("Page %zu is now \"%s\"\n", page, last->text[page]);
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
		char* token = NULL;
		uint8_t start = strtoul(line + 9, &token, 10), end = start;
		if(*token == '-'){
			end = strtoul(token + 1, NULL, 10);
		}

		if(end < start || end > 255 || start > 255){
			printf("Invalid message range: %s\n", line);
			return 1;
		}

		config.last_message = calloc(1, sizeof(message));
		if(!config.last_message){
			printf("Failed to allocate memory\n");
			return 1;
		}

		for(; start <= end; start++){
			if(config.message[start]){
				printf("Message collision at %d\n", start);
				return 1;
			}
			config.message[start] = config.last_message;
		}

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
			&& config.display_fd >= 0
			&& config.lines > 0){
		return 0;
	}
	printf("Configuration failed to validate\n");
	return 1;
}

void config_free(){
	size_t u, p;

	for(u = 0; u < (sizeof(config.message) / sizeof(message)); u++){
		if(config.message[u]){
			for(p = 0; p < config.message[u]->pages; p++){
				free(config.message[u]->text[p]);
			}
			free(config.message[u]->text);
			config.message[u]->text = NULL;
			config.message[u]->pages = 0;
		}
	}

	if(config.artnet_fd >= 0){
		close(config.artnet_fd);
	}

	if(config.display_fd >= 0){
		close(config.display_fd);
	}
}
