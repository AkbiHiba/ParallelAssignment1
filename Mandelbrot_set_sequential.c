#include <stdio.h>
#include <mpi.h>

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

int Mandlebrot()
{
 FILE *outputFile = fopen("mandelbrotSequential.ppm", "wb");
    if (outputFile == NULL) {
        perror("Unable to open output file");
        return 1;
    }

    fprintf(outputFile, "P6\n%d %d\n255\n", WIDTH, HEIGHT);

    float start_time= MPI_Wtime();
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // Convert pixel coordinate to complex number
            double real = RE_START + ((double)x / WIDTH) * (RE_END - RE_START);
            double imag = IM_START + ((double)y / HEIGHT) * (IM_END - IM_START);
            complex c;
            c.real= real;
            c.imag=imag;

            // Compute the number of iterations
            int m = cal_pixel(c);
            unsigned char color = 255 - (unsigned char)((m * 255) / MAX_ITER);

            // Write the color to the output file
            fputc(color, outputFile); 
            fputc(color, outputFile); 
            fputc(color, outputFile); 
        }
    }
    float end_time= MPI_Wtime();
    float ellapsed_time= end_time- start_time;
    printf("Execution time is for sequential code is: %f", ellapsed_time);


    return 0;
}


int main()
{
    MPI_Init(NULL, NULL);
    Mandlebrot();
    MPI_Finalize();
    return 0;
}