#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Bootstrap for themes.

// By default, concatenates all the resources and a table of contents
// into an object file with objcopy.

// Run nm <object> to get the symbols created by bootstrap.

// The user must make extern variable declarations like
// extern unsigned char _binary_myobject_data_start[];
// and link the object to get at the symbols.

// Pass the symbol like
// _binary_destname_o_start
// to BC_Theme::set_data to set an image table for a theme.


// Because of the need for runtime compilation of source files for OpenGL,
// it now supports loading a single resource and omitting the table of contents.




// Usage: bootstrap <dest object> <resource> * n




void append_contents(char *path,
	int data_offset,
	char *buffer,
	int *buffer_size)
{
	char string[1024];
	int i, j = 0;

	for(i = strlen(path) - 1;
		i > 0 && path[i] && path[i] != '/';
		i--)
		;

	if(path[i] == '/') i++;

	for(j = 0; path[i] != 0; i++, j++)
		string[j] = path[i];

	string[j] = 0;

	strcpy(buffer + *buffer_size, string);

	*buffer_size += strlen(string) + 1;

	*(int*)(buffer + *buffer_size) = data_offset;
	*buffer_size += sizeof(int);
}

int main(int argc, char *argv[])
{
	FILE *dest;
	FILE *src;
	int i;
	char *contents_buffer;
	int contents_size = 0;
	char *data_buffer;
	int data_size = 0;
	int data_offset = 0;
	char temp_path[1024];
	char out_path[1024];
	char *ptr;
	char system_command[1024];
	int current_arg = 1;
	int binary_mode = 0;
	int string_mode = 0;

	if(argc < 3)
	{
		fprintf(stderr, "Usage: bootstrap [-b] <dest object> <resource> * n\n");
		fprintf(stderr, "-b - omit table of contents just store resource.\n");
		fprintf(stderr, "-s - omit table of contents but append 0 to resource.\n");
		exit(1);
	}

	if(!strcmp(argv[current_arg], "-b"))
	{
		binary_mode = 1;
		current_arg++;
	}

	if(!strcmp(argv[current_arg], "-s"))
	{
		string_mode = 1;
		current_arg++;
	}

// Make object filename
	strcpy(temp_path, argv[current_arg]);
	strcpy(out_path, argv[current_arg++]);
	ptr = strchr(temp_path, '.');
	if(ptr)
	{
		*ptr = 0;
	}
	else
	{
		fprintf(stderr, "Dest path must end in .o\n");
		exit(1);
	}

	if(!(dest = fopen(temp_path, "w")))
	{
		fprintf(stderr, "Couldn't open dest file %s. %s\n",
			temp_path,
			strerror(errno));
		exit(1);
	}

	if(!(data_buffer = malloc(0x1000000)))
	{
		fprintf(stderr, "Not enough memory to allocate data buffer.\n");
		exit(1);
	}

	if(!(contents_buffer = malloc(0x100000)))
	{
		fprintf(stderr, "Not enough memory to allocate contents buffer.\n");
		exit(1);
	}

// Leave space for offset to data
	contents_size = sizeof(int);

// Read through all the resources, concatenate to dest file,
// and record the contents.
	while(current_arg < argc)
	{
		char *path = argv[current_arg++];
		if(!(src = fopen(path, "r")))
		{
			fprintf(stderr, "%s while opening %s\n", strerror(errno), path);
		}
		else
		{
			int size;
			fseek(src, 0, SEEK_END);
			size = ftell(src);
			fseek(src, 0, SEEK_SET);

			data_offset = data_size;

			if(!binary_mode && !string_mode)
			{
// Write size of image in data buffer
				*(data_buffer + data_size) = (size & 0xff000000) >> 24;
				data_size++;
				*(data_buffer + data_size) = (size & 0xff0000) >> 16;
				data_size++;
				*(data_buffer + data_size) = (size & 0xff00) >> 8;
				data_size++;
				*(data_buffer + data_size) = size & 0xff;
				data_size++;
			}

			int temp = fread(data_buffer + data_size, 1, size, src);
			data_size += size;
// Terminate string
			if(string_mode) data_buffer[data_size++] = 0;
			fclose(src);

			if(!binary_mode && !string_mode)
			{
// Create contents
				append_contents(path,
					data_offset,
					contents_buffer,
					&contents_size);
			}
		}
	}

	if(!binary_mode && !string_mode)
	{
// Finish off size of contents
		*(int*)(contents_buffer) = contents_size;
// Write contents
		fwrite(contents_buffer, 1, contents_size, dest);
	}

// Write data
	fwrite(data_buffer, 1, data_size, dest);
	fclose(dest);

// Run system command on it
	sprintf(system_command, "%s %s %s", BOOTSTRAP, temp_path, out_path);
	int temp = system(system_command);
	return 0;
}


