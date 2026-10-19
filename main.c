
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <SDL3/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef struct
{
    int width;
    int height;
    unsigned char *pixels;
} Image;

typedef struct
{
    char *path;
} FileDialogData;

static void SDLCALL image_file_callback(void *userdata, const char *const *filelist, int filter);
static Image load_ppm(FILE *input);
static Image load_png(FILE *input);
static bool read_ppm_token(FILE *input, char *buffer, size_t size);
static bool show_image(const Image *image, const char *title);
static void free_image(Image *image);
static bool lopenWithoutIamge(void);
static bool lopenWithIamge(char *argv);

static void SDLCALL image_file_callback(
    void *userdata,
    const char *const *filelist,
    int filter)
{
    (void)filter;

    FileDialogData *dialog = (FileDialogData *)userdata;

    if (dialog == NULL || filelist == NULL || filelist[0] == NULL)
    {
        printf("No image file selected.\n");
        return;
    }

    const char *selected_path = filelist[0];
    dialog->path = malloc(strlen(selected_path) + 1);

    if (dialog->path == NULL)
    {
        fprintf(stderr, "Error: Not enough memory for file path.\n");
        return;
    }

    strcpy(dialog->path, selected_path);
    printf("Selected image: %s\n", dialog->path);
}

// filter
static bool read_ppm_token(FILE *input, char *buffer, size_t size)
{
    int ch;
    size_t length = 0;

    if (size < 2)
    {
        return false;
    }

    for (;;)
    {
        ch = fgetc(input);

        if (ch == EOF)
        {
            return false;
        }

        if (ch == '#')
        {
            while ((ch = fgetc(input)) != EOF && ch != '\n')
            {
                // Skip comment
            }
            continue;
        }

        if (ch <= ' ')
        {
            continue;
        }
        break;
    }

    do
    {
        if (length + 1 >= size)
        {
            return false;
        }

        buffer[length++] = (char)ch;
        ch = fgetc(input);

    } while (ch != EOF && ch > ' ' && ch != '#');

    buffer[length] = '\0';

    if (ch == '#')
    {
        while ((ch = fgetc(input)) != EOF && ch != '\n')
        {
            // skip comment
        }
    }
    else if (ch == '\r')
    {
        int next = fgetc(input);

        if (next != '\n' && next != EOF)
        {
            ungetc(next, input);
        }
    }
    return true;
}

// load ppm image data
static Image load_ppm(FILE *input)
{
    Image image = {0};

    char token[64];
    int width;
    int height;
    int max_value;

    if (!read_ppm_token(input, token, sizeof(token)) ||
        strcmp(token, "P6") != 0)
    {
        fprintf(stderr, "Error: Expected a binary P6 PPM image.\n");
        return image;
    }

    if (!read_ppm_token(input, token, sizeof(token)))
    {
        fprintf(stderr, "Error: Missing PPM width.\n");
        return image;
    }

    char *end;
    long parsed_width = strtol(token, &end, 10);

    if (token[0] == '\0' || *end != '\0' ||
        parsed_width <= 0 || parsed_width > 10000)
    {
        fprintf(stderr, "Error: Invalid PPM width.\n");
        return image;
    }

    width = (int)parsed_width;

    if (!read_ppm_token(input, token, sizeof(token)))
    {
        fprintf(stderr, "Error: Missing PPM height.\n");
        return image;
    }

    long parsed_height = strtol(token, &end, 10);

    if (token[0] == '\0' || *end != '\0' ||
        parsed_height <= 0 || parsed_height > 10000)
    {
        fprintf(stderr, "Error: Invalid PPM height.\n");
        return image;
    }

    height = (int)parsed_height;

    if (!read_ppm_token(input, token, sizeof(token)))
    {
        fprintf(stderr, "Error: Missing PPM maximum color value.\n");
        return image;
    }

    long parsed_max = strtol(token, &end, 10);

    if (token[0] == '\0' || *end != '\0' || parsed_max != 255)
    {
        fprintf(stderr, "Error: PPM max value must be 255.\n");
        return image;
    }

    size_t pixel_count = (size_t)width * (size_t)height;

    if (pixel_count > SIZE_MAX / 3)
    {
        fprintf(stderr, "Error: Image is too large.\n");
        return image;
    }

    size_t data_size = pixel_count * 3;

    image.pixels = malloc(data_size);

    if (image.pixels == NULL)
    {
        fprintf(stderr, "Error: Not enough memory for PPM pixels.\n");
        return image;
    }

    if (fread(image.pixels, 1, data_size, input) != data_size)
    {
        fprintf(stderr, "Error: PPM pixel data is incomplete.\n");
        free(image.pixels);
        image.pixels = NULL;
        return image;
    }

    image.width = width;
    image.height = height;

    return image;
}

// load png image data 
static Image load_png(FILE *input)
{
    Image image = {0};
    int original_channels = 0;

    image.pixels = stbi_load_from_file(
        input,
        &image.width,
        &image.height,
        &original_channels,
        3);

    if (image.pixels == NULL)
    {
        fprintf(stderr, "PNG loading failed: %s\n",
                stbi_failure_reason());
        return image;
    }

    printf("Original image channels: %d\n", original_channels);

    return image;
}

// load the iamge to window
static bool show_image(const Image *image, const char *title)
{
    if (image->pixels == NULL ||
        image->width <= 0 ||
        image->height <= 0)
    {
        return false;
    }

    SDL_Window *window = SDL_CreateWindow(
        title,
        image->width,
        image->height,
        0);

    if (window == NULL)
    {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_Surface *surface = SDL_GetWindowSurface(window);

    if (surface == NULL)
    {
        fprintf(stderr, "Window surface failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return false;
    }

    const SDL_PixelFormatDetails *details =
        SDL_GetPixelFormatDetails(surface->format);

    if (details == NULL)
    {
        fprintf(stderr, "Pixel format failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return false;
    }

    for (int y = 0; y < image->height; y++)
    {
        for (int x = 0; x < image->width; x++)
        {
            size_t index =
                ((size_t)y * (size_t)image->width + (size_t)x) * 3;

            Uint8 r = image->pixels[index];
            Uint8 g = image->pixels[index + 1];
            Uint8 b = image->pixels[index + 2];

            Uint32 color = SDL_MapRGB(details, NULL, r, g, b);

            SDL_Rect pixel = {x, y, 1, 1};

            if (!SDL_FillSurfaceRect(surface, &pixel, color))
            {
                fprintf(stderr, "Pixel drawing failed: %s\n",
                        SDL_GetError());
                SDL_DestroyWindow(window);
                return false;
            }
        }
    }

    if (!SDL_UpdateWindowSurface(window))
    {
        fprintf(stderr, "Window update failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return false;
    }

    printf("Image size: %d x %d\n", image->width, image->height);
    printf("First pixel: R=%u G=%u B=%u\n",
           image->pixels[0],
           image->pixels[1],
           image->pixels[2]
        );

    printf("Hex color: #%02X%02X%02X\n",
           image->pixels[0],
           image->pixels[1],
           image->pixels[2]
        );

    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
        }

        SDL_Delay(10);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();

    return true;
}

// free Image
static void free_image(Image *image)
{
    if (image->pixels != NULL)
    {
        stbi_image_free(image->pixels);
        image->pixels = NULL;
    }
    image->width = 0;
    image->height = 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        lopenWithoutIamge();
    }
    else
    {
        char *path = argv[1];
        lopenWithIamge(path);
    }
    return 0;
}

// load the image data
static bool lopenWithIamge(char *argv)
{
    const char *filename = argv;
    FILE *input = fopen(filename, "rb");

    if (input == NULL)
    {
        perror("Could not open image");
        return EXIT_FAILURE;
    }

    // signature
    unsigned char header[8];
    size_t bytes_read = fread(header, 1, sizeof(header), input);

    static const unsigned char png_signature[8] = {
        0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

    bool is_png =
        bytes_read == sizeof(png_signature) &&
        memcmp(header, png_signature, sizeof(png_signature)) == 0;

    bool is_ppm =
        bytes_read >= 2 &&
        header[0] == 'P';
    // && header[1] == '6';

    if (!is_png && !is_ppm)
    {
        fprintf(stderr,
                "Unsupported image format. Use PNG or binary P6 PPM.\n");
        fclose(input);
        return EXIT_FAILURE;
    }

    if (fseek(input, 0, SEEK_SET) != 0)
    {
        fprintf(stderr, "Error: Cannot rewind image file.\n");
        fclose(input);
        return EXIT_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        fclose(input);
        return EXIT_FAILURE;
    }

    Image image = {0};

    if (is_png)
    {
        image = load_png(input);
    }
    else
    {
        image = load_ppm(input);
    }

    fclose(input);

    bool success = false;

    if (image.pixels != NULL)
    {
        success = show_image(
            &image,
            filename);
    }

    free_image(&image);
    SDL_Quit();
}

// Show window without image
static bool lopenWithoutIamge(void)
{
    SDL_Window *window = SDL_CreateWindow(
        "Image Viewer",
        900,
        600,
        0);

    if (window == NULL)
    {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

    if (renderer == NULL)
    {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    FileDialogData dialog = {NULL};

    const SDL_DialogFileFilter filters[] = {
        {"Images", "png;ppm"}};

    bool running = true;
    bool dialog_open = false;
    SDL_Event event;

    SDL_FRect button = {
        340.0f, 240.0f,
        220.0f, 130.0f};

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }

            // click on add image button to add image
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                event.button.button == SDL_BUTTON_LEFT)
            {
                float mouse_x = (float)event.button.x;
                float mouse_y = (float)event.button.y;

                if (!dialog_open &&
                    mouse_x >= button.x &&
                    mouse_x <= button.x + button.w &&
                    mouse_y >= button.y &&
                    mouse_y <= button.y + button.h)
                {
                    dialog_open = true;

                    SDL_ShowOpenFileDialog(
                        image_file_callback,
                        &dialog,
                        window,
                        filters,
                        1,
                        NULL,
                        false
                    );
                }
            }

            // Drop image on the window
            if (event.type == SDL_EVENT_DROP_FILE &&
                event.drop.data != NULL)
            {
                char *path = SDL_strdup(event.drop.data);

                if (path != NULL)
                {
                    running = false;

                    SDL_DestroyRenderer(renderer);
                    SDL_DestroyWindow(window);

                    (void)lopenWithIamge(path);

                    SDL_free(path);
                    return true;
                }
            }
        }

        if (dialog.path != NULL)
        {
            char *path = dialog.path;
            dialog.path = NULL;

            running = false;

            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);

            (void)lopenWithIamge(path);

            free(path);
            return true;
        }

        SDL_SetRenderDrawColor(renderer, 245, 247, 250, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 225, 232, 242, 255);
        SDL_RenderFillRect(renderer, &button);

        SDL_SetRenderDrawColor(renderer, 50, 100, 200, 255);

        SDL_FRect horizontal = {
            415.0f, 290.0f,
            70.0f, 8.0f};

        SDL_FRect vertical = {
            446.0f, 259.0f,
            8.0f, 70.0f};

        SDL_RenderFillRect(renderer, &horizontal);
        SDL_RenderFillRect(renderer, &vertical);
        SDL_SetRenderDrawColor(renderer, 35, 45, 60, 255);
        SDL_RenderDebugText(renderer, 414.0f, 345.0f, "ADD IMAGE");
        SDL_RenderPresent(renderer);

        SDL_Delay(10);
    }

    free(dialog.path);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return true;
}
