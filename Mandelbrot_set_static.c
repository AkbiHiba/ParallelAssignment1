#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>


#define WIDTH 600
#define HEIGHT 400
// Plot window
#define RE_START -2
#define RE_END 1
#define IM_START -1
#define IM_END 1

#define MAX_ITER 100

//object for the imagianry numbrs
typedef struct complex {
    float real;
    float imag;
}complex;

int cal_pixel(complex c)
{
    float temp, lengthsq;
    int max = 256;
    int count = 0; 
    complex z;
    z.real = 0; 
    z.imag = 0;
    
    do {
        temp = z.real * z.real - z.imag * z.imag + c.real;
        z.imag = 2 * z.real * z.imag + c.imag;
        z.real = temp;
        lengthsq = z.real * z.real + z.imag * z.imag;
        count++;
    } while ((lengthsq < 4.0) && (count < max));
    return count;
}



int main()
{
    MPI_Init(NULL, NULL);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);


    int region_width=WIDTH;
    int region_height=HEIGHT/world_size; //height is divided eqaully among regions
    float start_time= MPI_Wtime();

    unsigned char* MandelbrotRegion = (unsigned char*)malloc(WIDTH * region_height * sizeof(unsigned char));

    for (int y = region_height*world_rank; y < region_height*world_rank+region_height; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            double real = RE_START + ((double)x / WIDTH) * (RE_END - RE_START);
            double imag = IM_START + ((double)y / HEIGHT) * (IM_END - IM_START);
            complex c;
            c.real= real;
            c.imag=imag;

            int m = cal_pixel(c);
            
            unsigned char color = 255 - (unsigned char)((m * 255) / MAX_ITER);
            MandelbrotRegion[(y - region_height*world_rank) * WIDTH + x] = color;
     
        }
    }

    
    unsigned char* Mandelbrot = NULL;
    Mandelbrot = (unsigned char*)malloc(WIDTH * HEIGHT * sizeof(unsigned char));

    MPI_Gather(MandelbrotRegion, WIDTH * region_height, MPI_UNSIGNED_CHAR, Mandelbrot, WIDTH *region_height, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    
    float end_time= MPI_Wtime();
    float ellapsed_time= end_time- start_time;

    if(world_rank==0){
        printf("Execution time is for dynamic is: %f",  ellapsed_time);
        FILE *outputFile = fopen("mandelbrotStatic.ppm", "wb");
        if (outputFile == NULL) {
            perror("Unable to open output file");
            return 1;
        }
        fprintf(outputFile, "P6\n%d %d\n255\n", WIDTH, HEIGHT);

        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                fputc(Mandelbrot[y * WIDTH + x], outputFile); 
                fputc(Mandelbrot[y * WIDTH + x], outputFile);
                fputc(Mandelbrot[y * WIDTH + x], outputFile);
                
            }
    }
    fclose(outputFile);
    }
    
    MPI_Finalize();
    return 0;
}