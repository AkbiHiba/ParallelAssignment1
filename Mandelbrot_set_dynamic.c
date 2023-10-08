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
#define TAG_TASK 1
#define TAG_RESULT 2


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
    int region_height=4; //height is divided eqaully among regions
    unsigned char* MandelbrotRegion = (unsigned char*)malloc(WIDTH * region_height * sizeof(unsigned char));
    unsigned char* Mandelbrot = NULL;
    float start_time= MPI_Wtime();
    if (world_rank == 0) {
        
        Mandelbrot = (unsigned char*)malloc(WIDTH * HEIGHT * sizeof(unsigned char));

        MPI_Request req;
        int start_region;
        int num_tasks = 100;
        int next_region = 0;

        //first iteration to ditribute tasks
        for(int i=1; i<world_size;i++){
            start_region = i * region_height;
            //send start boundaries
            MPI_Isend(&start_region, 1, MPI_INT, i, TAG_TASK, MPI_COMM_WORLD, &req);
            num_tasks--;
            next_region=(100-num_tasks)*region_height;
        }
        
        //for ditributing new tasks
        while (num_tasks>0) {
           
            MPI_Status status;
            int slave_rank;
            MPI_Recv(MandelbrotRegion, WIDTH * region_height, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD, &status);        
            if (next_region * region_height < HEIGHT) {
                start_region = next_region * region_height;
                slave_rank = status.MPI_SOURCE;
                MPI_Isend(&start_region, 1, MPI_INT, slave_rank, TAG_TASK, MPI_COMM_WORLD, &req);
                num_tasks--;
                next_region=(100-num_tasks)*region_height;
            }
        }
        //termiante evryone when the number of tasks ended
        for (int i = 1; i < world_size; i++) {
            start_region = -1;
            MPI_Isend(&start_region, 1, MPI_INT, i, TAG_TASK, MPI_COMM_WORLD, &req);
        }
    
    }
    else if (world_rank!=0){

        //code for slaves

        MPI_Status status;
        int region_start;
        
        while (true) {
            MPI_Recv(&region_start, 1, MPI_INT, 0, TAG_TASK, MPI_COMM_WORLD, &status);
            //if received terminal signal, end processor work
            if (region_start == -1) {
                break;  
            }
            
            for (int y = region_start; y < region_start+region_height; ++y) {
                for (int x = 0; x < WIDTH; ++x) {
                    // Convert pixel coordinate to complex number
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
            
            MPI_Send(MandelbrotRegion, WIDTH * region_height, MPI_UNSIGNED_CHAR, 0, TAG_RESULT, MPI_COMM_WORLD);
        }
    }

    MPI_Gather(MandelbrotRegion, WIDTH * region_height, MPI_UNSIGNED_CHAR, Mandelbrot, WIDTH *region_height, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    float end_time= MPI_Wtime();
    float ellapsed_time= end_time- start_time;
    printf("Execution time is for dynamic code is: %f", ellapsed_time);

    if (world_rank == 0) {
        FILE *outputFile = fopen("mandelbrotDynamic.ppm", "wb");
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