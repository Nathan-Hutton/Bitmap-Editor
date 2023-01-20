#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

// Struct for an image, containing its dimensions and pixel data
struct bitmap
{
	int width;
	int height;
	int *pixels;
};

const int DIB_HEADER_SIZE = 14;
const int BITMAP_HEADER_SIZE = 40;

// Make "byte" mean "unsigned char"
typedef unsigned char byte;

// Rotate image 90 degrees counter clockwise
void bitmap_rotate(struct bitmap *bmp);

// Squash image width and height
void bitmap_shrink(struct bitmap *bmp);

// Reflects image
void bitmap_reflect(struct bitmap *bmp);

// skew the bitmap as if the bottom of the image was pulled to the left
void bitmap_skew(struct bitmap *bmp);

// Squashes image in half
void bitmap_squash(struct bitmap *bmp);

// Mirrors an image
void bitmap_mirror(struct bitmap *bmp);

// Turn image into greyscale
void bitmap_to_greyscale(struct bitmap *bmp);

// Posterize image
void bitmap_posterize(struct bitmap *bmp);

// Calculates the stride of a .bmp file.
// (The stride is how many bytes of memory a single row of
// the image requires.)
inline int bmp_file_stride(struct bitmap *bmp);

// Calculates the total size that a .bmp file for this bitmap
// would need (in bytes)
inline int bmp_file_size(struct bitmap *bmp);

// Opens the file with the given name and maps it into memory
// so that we can access its contents through pointers.
void *map_file_for_reading(char *filename);

// Opens (and creates if necessary) the file with the given name
// and maps it into memory so that we can access its contents
// through pointers.
void *map_file_for_writing(char *filename, int file_size);

// Takes the contents of a bitmap file (bmp_file) and reads
// its data, filling in the struct bitmap pointed to by bmp.
// Returns 0 if everything worked, -1 if the file data isn't
// valid.
int read_bitmap(void *bmp_file, struct bitmap *bmp);

// Opposite of read_bitmpa()
void write_bitmap(void *bmp_file, struct bitmap *bmp);

// Converts between a packed pixel (0xRRGGBB) and its components.
void rgb_to_pixel(int *p, int r, int g, int b);
void pixel_to_rgb(int p, int *r, int *g, int *b);

/* Please note: if your program has a main() function, then
 * the test programs given to you will not run (your main()
 * will override the test program's). When running a test,
 * make sure to either comment out or rename your main()
 * function!
 */


int main(int argc, char *argv[])
{
    char *reader_pointer = map_file_for_reading(argv[1]);
    struct bitmap chosen_bitmap;
    read_bitmap(reader_pointer, &chosen_bitmap);
    munmap(reader_pointer, reader_pointer[0]);

    char answer[20];
    while(answer[0] != 'Q' && answer[0] != 'q')
    {
        printf("Menu:\n");
        printf("    G) Make grayscale\n");
        printf("    P) Posterize\n");
        printf("    U) Squash\n");
        printf("    M) Mirror\n");
        printf("    R) Reflect\n");
        printf("    K) Skew\n");
        printf("    N) Shrink\n");
        printf("    O) Rotate\n");
        printf("    S) Save\n");
        printf("    Q) Quit\n");
        printf("What would you like to do? ");

        scanf("%s", answer);

        if(answer[0] == 'G' || answer[0] == 'g')
        {
            printf("\nGrayscale selected\n\n");
            bitmap_to_greyscale(&chosen_bitmap);
        }
        else if(answer[0] == 'P' || answer[0] == 'p')
        {
            printf("\nPosterize selected\n\n");
            bitmap_posterize(&chosen_bitmap);
        }
        else if(answer[0] == 'U' || answer[0] == 'u')
        {
            printf("\nSquash selected\n\n");
            bitmap_squash(&chosen_bitmap);
        }
        else if(answer[0] == 'M' || answer[0] == 'm')
        {
            printf("\nMirror selected\n\n");
            bitmap_mirror(&chosen_bitmap);
        }
        else if(answer[0] == 'R' || answer[0] == 'r')
        {
            printf("\nReflect selected\n\n");
            bitmap_reflect(&chosen_bitmap);
        }
        else if(answer[0] == 'K' || answer[0] == 'k')
        {
            printf("\nSkew selected\n\n");
            bitmap_skew(&chosen_bitmap);
        }
        else if(answer[0] == 'N' || answer[0] == 'n')
        {
            printf("\nShrink selected\n\n");
            bitmap_shrink(&chosen_bitmap);
        }
        else if(answer[0] == 'O' || answer[0] == 'o')
        {
            printf("\nRotate selected\n\n");
            bitmap_rotate(&chosen_bitmap);
        }
        else if(answer[0] == 'S' || answer[0] == 's')
        {
            char *filename;
            printf("\nEnter filename: ");
            scanf("%s", filename);
            printf("\nSaving to %s...\n", filename);

            char *writer_pointer = map_file_for_writing(filename, bmp_file_size(&chosen_bitmap));
            write_bitmap(writer_pointer, &chosen_bitmap);
            munmap(writer_pointer, writer_pointer[0]);
            printf("Saved!\n\n");
        }
    }
    return 0;
}

void bitmap_rotate(struct bitmap *bmp)
{
    int *new_pixels = (int*) malloc(bmp->width * bmp->height * sizeof(int));
    int pixel;

    for(int y = 0; y < bmp->height; y++)
    {
        for(int x = 0; x < bmp->width; x++)
        {
            pixel = bmp->pixels[y * bmp->width + x];
            new_pixels[(bmp->width - 1 - x) * bmp->height + y] = pixel;
        }
    }
    free(bmp->pixels);
    bmp->pixels = new_pixels;
    int old_width = bmp->width;
    bmp->width = bmp->height;
    bmp->height = old_width;
}

void bitmap_shrink(struct bitmap *bmp)
{
    int new_width = bmp->width / 2;
    int new_height = bmp->height / 2;
    int *new_pixels = (int*) malloc(new_width * new_height * sizeof(int));

    int first_pixel, second_pixel, third_pixel, fourth_pixel;
    int pixel;
    int r1, r2, r3, r4, g1, g2, g3, g4, b1, b2, b3, b4;
    int final_r, final_g, final_b;

    for(int y = 0; y < new_height; y++)
    {
        for(int x = 0; x < new_width; x++)
        {
            first_pixel = bmp->pixels[(y*2) * bmp->width + (2*x)];
            pixel_to_rgb(first_pixel, &r1, &g1, &b1);

            second_pixel = bmp->pixels[(y*2) * bmp->width + ((2*x) + 1)];
            pixel_to_rgb(second_pixel, &r2, &g2, &b2);

            third_pixel = bmp->pixels[((y*2) + 1) * bmp->width + (2*x)];
            pixel_to_rgb(third_pixel, &r3, &g3, &b3);

            fourth_pixel = bmp->pixels[((y*2) + 1) * bmp->width + ((2*x) + 1)];
            pixel_to_rgb(fourth_pixel, &r4, &g4, &b4);

            final_r = (r1 + r2 + r3 + r4) / 4;
            final_g = (g1 + g2 + g3 + g4) / 4;
            final_b = (b1 + b2 + b3 + b4) / 4;

            rgb_to_pixel(&pixel, final_r, final_g, final_b);
            new_pixels[y * new_width + x] = pixel;
        }
    }
    free(bmp->pixels);
    bmp->pixels = new_pixels;
    bmp->width = new_width;
    bmp->height = new_height;
}

void bitmap_skew(struct bitmap *bmp)
{
    int *new_pixels = (int*) malloc(bmp->width * bmp->height * sizeof(int));
    int pixel;
    for(int y = 0; y < bmp->height; y++)
    {
        for(int x = 0; x < bmp->width; x++)
        {
            pixel = bmp->pixels[y * bmp->width + x];
            if(x < y)
            {
                new_pixels[y * bmp->width + (bmp->width - 1 + (x-y))] = pixel;
            }
            else
            {
                new_pixels[y * bmp->width + (x - y)] = pixel;
            }
        }
    }
    free(bmp->pixels);
    bmp->pixels = new_pixels;
}

void bitmap_reflect(struct bitmap *bmp)
{
    int *new_pixels = (int*) malloc(bmp->width * bmp->height * sizeof(int));
    int pixel;
    for(int y = 0; y < bmp->height; y++)
    {
        for(int x = 0; x < bmp->width; x++)
        {
            pixel = bmp->pixels[y * bmp->width + x];
            new_pixels[y * bmp->width + (bmp->width - 1 - x)] = pixel;
        }
    }
    free(bmp->pixels);
    bmp->pixels = new_pixels;
}

void bitmap_squash(struct bitmap *bmp)
{
    int old_width = bmp->width;
    int new_width = old_width / 2;
    int *new_pixels = (int*) malloc(new_width * bmp->height * sizeof(int));

    int first_pixel;
    int second_pixel;
    int pixel;
    int r1, r2, r3, g1, g2, g3, b1, b2, b3;
    for(int y = 0; y < bmp->height; y++)
    {
        for(int x = 0; x < new_width; x++)
        {
            first_pixel = bmp->pixels[y * old_width + (2 * x)];
            pixel_to_rgb(first_pixel, &r1, &g1, &b1);

            second_pixel = bmp->pixels[y * old_width + ((2 * x) + 1)];
            pixel_to_rgb(second_pixel, &r2, &g2, &b2);

            r3 = (r1 + r2) / 2;
            g3 = (g1 + g2) / 2;
            b3 = (b1 + b2) / 2;

            rgb_to_pixel(&pixel, r3, g3, b3);
            new_pixels[y * new_width + x] = pixel;
        }
    }
    free(bmp->pixels);
    bmp->pixels = new_pixels;
    bmp->width = new_width;
}

void bitmap_mirror(struct bitmap *bmp)
{
    int old_width = (*bmp).width;
    int new_width = old_width * 2;
    int *new_pixels = (int*) malloc(new_width * (*bmp).height * sizeof(int));

    int pixel;
    for(int y = 0; y < (*bmp).height; y++)
    {
        for(int x = 0; x < old_width; x++)
        {
            pixel = (*bmp).pixels[y * old_width + x];
            new_pixels[y * new_width + x] = pixel;
            new_pixels[y * new_width + (new_width - 1 - x)] = pixel;
        }
    }
    free((*bmp).pixels);
    (*bmp).pixels = new_pixels;
    (*bmp).width = new_width;
}

void bitmap_to_greyscale(struct bitmap *bmp)
{
    int pixel, r, g, b, average;
    int pixel_num = bmp->width * bmp->height;
    for(int i = 0; i < pixel_num; i++)
    {
        pixel = bmp->pixels[i];
        pixel_to_rgb(pixel, &r, &g, &b);
        average = (r + b + g) / 3;
        r = g = b = average;
        rgb_to_pixel(&pixel, r, g, b);
        bmp->pixels[i] = pixel;
    }
}

void bitmap_posterize(struct bitmap *bmp)
{
    int pixel, r, g, b;
    int *colors[3] = {&r, &g, &b};
    int pixel_num = bmp->height * bmp->width;
    for(int i = 0; i < pixel_num; i++)
    {
        pixel = bmp->pixels[i];
        pixel_to_rgb(pixel, &r, &g, &b);
        for(int j = 0; j < 3; j++)
        {
            if(*colors[j] < 32)
            {
                *colors[j] = 0;
            }
            else if(*colors[j] <= 95)
            {
                *colors[j] = 64;
            }
            else if(*colors[j] <= 159)
            {
                *colors[j] = 128;
            }
            else if(*colors[j] <= 223)
            {
                *colors[j] = 192;
            }
            else if(*colors[j] >= 224)
            {
                *colors[j] = 255;
            }
        }
        rgb_to_pixel(&pixel, r, g, b);
        bmp->pixels[i] = pixel;
    }
}

int bmp_file_stride(struct bitmap *bmp)
{
    return (24 * bmp->width + 31) / 32 * 4;
}

int bmp_file_size(struct bitmap *bmp)
{
    int stride = bmp_file_stride(bmp);
    return DIB_HEADER_SIZE
        + BITMAP_HEADER_SIZE
        + stride * bmp->height;
}

void *map_file_for_reading(char *filename)
{
    // A) Use open() to open the file for reading.
    int fd = open(filename, O_RDONLY);
    if(fd == -1){
        perror(NULL);
        return NULL;
    }
    // B) Use fstat() to determine the size of the file.
    struct stat buffer;
    int fd2 = fstat(fd, &buffer);
    if(fd2 == -1){
        perror(NULL);
        return NULL;
    }
    long file_size = buffer.st_size;

    // C) Call mmap() to map the file into memory.
    char* pointer = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
    if(pointer == MAP_FAILED){
        perror(NULL);
        return NULL;
    }

    // D) Close the file using close().
    close(fd);

    // E) Return the pointer returned from mmap().
    return pointer;
}

void *map_file_for_writing(char *filename, int file_size)
{
    // A) Use open() to open the file for writing.
    int fd = open(filename, O_RDWR | O_CREAT, 0644);
    if(fd == -1)
    {
        perror(NULL);
        return NULL;
    }
    // B) Use ftruncate() to set the size of the file.
    int fd2 = ftruncate(fd, file_size);
    if(fd2 == -1)
    {
        perror(NULL);
        return NULL;
    }
    // C) Call mmap() to map the file into memory.
    char* pointer = mmap(NULL, file_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if(pointer == MAP_FAILED)
    {
        perror(NULL);
        return NULL;
    }
    // D) Close the file using close().
    close(fd);

    // E) Return the pointer returned from mmap().
    return pointer;

}

int read_bitmap(void *bmp_file, struct bitmap *bmp)
{
    // Cast bmp_file to a byte * so we can access it
    // byte by byte.
    byte *file = (byte *) bmp_file;

    // Check the magic: it should start with "BM"
    if(file[0] != 'B' || file[1] != 'M')
    {
        printf("ERROR: Not a Bitmap");
        return -1;
    }
    if(*((short *) (file + 28)) != 24)
    {
        printf("Error: Color depth is not 24");
        return -1;
    }
    if(*((int *) (file + 30)) != 0)
    {
        printf("Error: Compression method is not 0");
        return -1;
    }

    bmp->width = *((int *) (file + 18));
    bmp->height = *((int *) (file + 22));

    // Size of pixel array
    int image_size = bmp->height * bmp->width;

    // Allocating memory for bitmap pixels
    bmp->pixels = (int*) malloc(image_size * sizeof(int));

    // Stride is for the bytes of the color and for the 0's used to make the bytes a multiple of 4
    int stride = bmp_file_stride(bmp);

    // Initialzing bmp->pixel data
    int offset = *((int *) (file + 10));
    int pixel;
    for(int y = 0; y < bmp->height; y++)
    {
        int row_index = 0;
        for(int x = 0; x < bmp->width; x++)
        {
            int b = *(file + offset++);
            int g = *(file + offset++);
            int r = *(file + offset++);
            rgb_to_pixel(&pixel, r, g, b);
            bmp->pixels[image_size - (bmp->width * (y + 1)) + row_index] = pixel;
            row_index++;
        }
        offset += stride % 3;
    }

    return 0;
}

void write_bitmap(void *bmp_file, struct bitmap *bmp)
{
    // Cast bmp_file to a byte * so we can access it
    // byte by byte.
    byte *file = (byte *) bmp_file;

    // bmp variables
    int pixel_offset = 54;
    int header_size = BITMAP_HEADER_SIZE;
    int stride = bmp_file_stride(bmp);
    int image_size = bmp->height * bmp_file_stride(bmp);
    file[0] = 'B';
    file[1] = 'M';
    *((int *) (file + 2)) = bmp_file_size(bmp);
    file[10] = pixel_offset;
    file[14] = header_size;
    *((int *) (file + 18)) = bmp->width;
    *((int *) (file + 22)) = bmp->height;
    file[26] = 1;
    file[28] = 24;
    file[46] = 0;
    *((int *) (file + 34)) = image_size;

    int b;
    int g;
    int r;
    int index = 0;

    for(int y = bmp->height - 1; y >= 0; y--)
    {
        for(int x = 0; x < bmp->width; x++)
        {
            int pixel = bmp->pixels[y * bmp->width + x];
            pixel_to_rgb(pixel, &r, &g, &b);
            file[pixel_offset + index++] = b;
            file[pixel_offset + index++] = g;
            file[pixel_offset + index++] = r;
        }
        index += stride % 3;
    }
}

void rgb_to_pixel(int *p, int r, int g, int b)
{
    // Pack r, g, and b into an int value and save
    // into what p points to
    *p = ((r & 0xFF) << 16) + ((g & 0x00FF) << 8) + (b & 0x0000FF);
}

void pixel_to_rgb(int p, int *r, int *g, int *b)
{
    // Separate the pixel p into its components
    // and save in the pointers
    *r = p >> 16;
    *g = (p & 0x00FF00) >> 8;
    *b = (p & 0xFF);
}

