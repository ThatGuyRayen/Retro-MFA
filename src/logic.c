#include "retromfa.h"

t_data g_app = {NULL, 0ul, NULL, {NULL, 0}};
volatile sig_atomic_t g_should_exit = 0;


void destroy_all_windows(t_data *app)
{
    t_list *iter = app->images;
    while (iter)
    {
        if (iter->content.win_ptr && app->mlx_ptr)
        {
            mlx_destroy_window(app->mlx_ptr, iter->content.win_ptr);
            iter->content.win_ptr = NULL; // Mark as destroyed
        }
        iter = iter->next;
    }
}

void deleteList(t_list **head)
{
    if (!head || !*head)
        return;

    t_list *current = *head;
    t_list *next;

    while (current)
    {
        next = current->next;
        
        // Destroy the MLX image if it exists
        if (current->content.img_ptr && g_app.mlx_ptr)
        {
            mlx_destroy_image(g_app.mlx_ptr, current->content.img_ptr);
            current->content.img_ptr = NULL;
        }
        
        // DO NOT free content.data - it's managed by MLX!
        // if (current->content.data) {
        //    free(current->content.data);
        // }
        
        // Free the list node
        free(current);
        current = next;
    }
    
    *head = NULL;
}

t_list *createNode(void *mlx_img,char *data ,size_t width, size_t height)
{
	(void)data;
    t_list *node = calloc(1, sizeof(t_list));
    if (!node)
    {
        return NULL;
    }

    node->content = (t_image){
        .img_ptr = mlx_img,
        .data = NULL,  // Important: Don't store the MLX data pointer
        .width = width,
        .height = height,
        .win_ptr = NULL
    };
    node->next = NULL;
    
    g_app.nb_images++;  // Update image count if using global counter
    
    return node;
}

void cleanup_and_exit(int code)
{
    static bool is_cleaning = false;
    
    if (is_cleaning) return;
    is_cleaning = true;

    // 1. Destroy all windows first (windows contain images)
    destroy_all_windows(&g_app);
    
    // 2. Delete the image list (will destroy remaining images)
    deleteList(&g_app.images);
    g_app.nb_images = 0;  // Reset counter
    
    // 3. Clean up input data
    if (g_app.input.content)
    {
        free(g_app.input.content);
        g_app.input.content = NULL;
        g_app.input.size = 0;
    }
    
    // 4. Destroy MLX display (must be last)
    if (g_app.mlx_ptr)
    {
        mlx_destroy_display(g_app.mlx_ptr);
        free(g_app.mlx_ptr);
        g_app.mlx_ptr = NULL;
    }
    
    _exit(code);
}
int on_close(void)
{
    cleanup_and_exit(0);
    return 0;
}

void	render_rgb888(char *dst, const unsigned char *src, int width, int height)
{
	for (int i = 0; i < width * height * 3; i += 3)
	{
		uint32_t pixel = (src[i + 2] << 16) | (src[i + 1] << 8) | (src[i]);
		*(uint32_t *)(dst + (i / 3) * 4) = pixel;
	}
}

void	add_rgb888_image(const char *src, int width, int height)
{
	int bpp, line_size, endian;
	void *img = mlx_new_image(g_app.mlx_ptr, width, height);
	if (!img)
	{
		fprintf(stderr, "Failed to create image\n");
		cleanup_and_exit(1);
	}
	char *dst = mlx_get_data_addr(img, &bpp, &line_size, &endian);
	render_rgb888(dst, (unsigned char *)src, width, height);
	t_list *entry = createNode(img, dst, width, height);
	if (!entry)
	{
		fprintf(stderr, "Failed to allocate image node\n");
		mlx_destroy_image(g_app.mlx_ptr, img);
		cleanup_and_exit(1);
	}
	appendNode(&g_app, entry);
}

void	render_rgb555(char *dst, const unsigned char *src, int width, int height)
{
	for (int i = 0; i < width * height * 2; i += 2)
	{
		uint16_t raw = (src[i] << 8) | src[i + 1];
		int r = (raw >> 10) & 0x1F;
		int g = (raw >> 5) & 0x1F;
		int b = raw & 0x1F;
		uint32_t pixel = (r * 255 / 31 << 16) | (g * 255 / 31 << 8) | (b * 255 / 31);
		*(uint32_t *)(dst + (i / 2) * 4) = pixel;
	}
}

void	add_rgb555_image(const char *src, int width, int height)
{
	int bpp, line_size, endian;
	void *img = mlx_new_image(g_app.mlx_ptr, width, height);
	if (!img)
	{
		fprintf(stderr, "Failed to create image\n");
		cleanup_and_exit(1);
	}
	char *dst = mlx_get_data_addr(img, &bpp, &line_size, &endian);
	render_rgb555(dst, (unsigned char *)src, width, height);
	t_list *entry = createNode(img, dst, width, height);
	if (!entry)
	{
		fprintf(stderr, "Failed to allocate image node\n");
		mlx_destroy_image(g_app.mlx_ptr, img);
		cleanup_and_exit(1);
	}
	appendNode(&g_app, entry);
}

void	display_all_images(void)
{
	t_list *node = g_app.images;
	while (node)
	{
		node->content.win_ptr = mlx_new_window(g_app.mlx_ptr, node->content.width, node->content.height, "RetroMFA");
		if (!node->content.win_ptr)
		{
			fprintf(stderr, "Failed to create window\n");
			cleanup_and_exit(1);
		}
		mlx_put_image_to_window(g_app.mlx_ptr, node->content.win_ptr, node->content.img_ptr, 0, 0);
		mlx_hook(node->content.win_ptr, 2, 1L << 0, on_close, NULL);
		mlx_hook(node->content.win_ptr, 17, 0, on_close, NULL);
		node = node->next;
	}
}

void	handle_signal(int sig)
{
	if (sig == SIGINT)
		g_should_exit = 1;
}

int main(int argc, char **argv)
{
    if (argc != 2 || argv[1] == NULL || argv[1][0] == '\0')
    {
        printf("Usage: %s <filename.mfa>\n", argv[0]);
        return 1;
    }

    char *extension = strrchr(argv[1], '.');
    if (extension == NULL)
    {
        fprintf(stderr, "Error: Please enter a valid filename with extension.\n");
        return 1;
    }

    if (strncmp(extension, ".mfa", 4) != 0 || strlen(argv[1]) <= 4 || strlen(extension) != 4)
    {
        fprintf(stderr, "Error: Only '.mfa' files are supported.\n");
        return 1;
    }

    signal(SIGINT, handle_signal);

    g_app.mlx_ptr = mlx_init();
    if (!g_app.mlx_ptr)
    {
        fprintf(stderr, "Error: Unable to initialize MLX.\n");
        return 1;
    }

    readAllBytes(argv[1], &g_app.input);
    if (g_app.input.content == NULL)
    {
        fprintf(stderr, "Error: Unable to read file '%s'.\n", argv[1]);
        cleanup_and_exit(1);
    }

    int images = 0;
    for (size_t i = 3; i < g_app.input.size && images < 100; ++i)
    {
        if ((g_app.input.content[i - 3] == 0 && g_app.input.content[i - 2] == 6) &&
            (g_app.input.content[i - 1] == 16 && g_app.input.content[i] == 0))
        {
            int width = TO_INT(g_app.input.content[i - 6]) + (TO_INT(g_app.input.content[i - 5]) << 8);
            int height = TO_INT(g_app.input.content[i - 4]) + (TO_INT(g_app.input.content[i - 3]) << 8);

            if (width <= 0 || height <= 0)
            {
                fprintf(stderr, "Warning: Invalid image size found, skipping.\n");
                continue;
            }

            add_rgb555_image(g_app.input.content + i + 17, width, height);
            images++;
        }
        else if ((g_app.input.content[i - 3] == 0 && g_app.input.content[i - 2] == 4) &&
                 (g_app.input.content[i - 1] == 16 && g_app.input.content[i] == 0))
        {
            int width = TO_INT(g_app.input.content[i - 6]) + (TO_INT(g_app.input.content[i - 5]) << 8);
            int height = TO_INT(g_app.input.content[i - 4]) + (TO_INT(g_app.input.content[i - 3]) << 8);

            if (width <= 0 || height <= 0)
            {
                fprintf(stderr, "Warning: Invalid image size found, skipping.\n");
                continue;
            }

            add_rgb888_image(g_app.input.content + i + 17, width, height);
            images++;
        }
    }

    display_all_images();
    //mlx_loop_hook(g_app.mlx_ptr, quick_exit, NULL);
    mlx_loop(g_app.mlx_ptr);

    cleanup_and_exit(0);
    return 0;
}


