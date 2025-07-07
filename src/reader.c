#include "retromfa.h"


static char	*my_realloc(char *ptr, size_t size, char *buffer, size_t readSize)
{
	char *result = calloc(size + readSize, sizeof (char));

	if (!result)
	{
		free(ptr);
		return NULL;
	}

	if (ptr)
	{
		memcpy(result, ptr, size);
	}
	memcpy(result + size, buffer, readSize);
	free(ptr);
	return result;
}

void	readAllBytes(const char *filename, t_input *input)
{
	const int fd = open(filename, O_RDONLY);

	if (fd == -1)
	{
		return ;
	}

	size_t size = 0, readSize = 0;
	char buffer[BUFFER_SIZE + 1];
	char *result = NULL;

	bzero(buffer, BUFFER_SIZE + 1);
	while ((readSize = read(fd, buffer, BUFFER_SIZE)) > 0)
	{
		result = my_realloc(result, size, buffer, readSize);
		size += readSize;
	}
	close(fd);
	input->content = result;
	input->size = size;
}
