/**
 *  \file mandelbrot_joe.cc
 *
 *  \brief Implement your parallel mandelbrot set in this file.
 */

 /**
  *  \file mandelbrot_serial.cc
  *  \brief HW 2: Mandelbrot set serial code
  */


 #include <iostream>
 #include <string.h>
 #include <cstring>
 #include <cstdlib>
 #include "render.hh"
#include <mpi.h>
 using namespace std;
 #include <math.h>

 #define WIDTH 1000
 #define HEIGHT 1000

 int mandelbrot(double x, double y) {
   int maxit = 511; /*max number if iterations to perform on the iterative equation*/
   double cx = x;
   double cy = y;
   double newx, newy;

   int it = 0;
   for (it = 0; it < maxit && (x*x + y*y) < 4; ++it) {
     newx = x*x - y*y + cx;
     newy = 2*x*y + cy;
     x = newx;
     y = newy;
   }
   return it;
 }

 int
 main(int argc, char* argv[]) {
   double minX = -2.1;
   double maxX = 0.7;
   double minY = -1.25;
   double maxY = 1.25;
   FILE * fp = NULL;
   double t_start, t_elapsed;


   int height, width;
   int rank, size;
   if (argc == 3) {
     height = atoi (argv[1]);
     width = atoi (argv[2]);
     cout << height << endl;
     cout << width << endl;
     assert (height > 0 && width > 0);
   } else {
     fprintf (stderr, "usage: %s <height> <width>\n", argv[0]);
     fprintf (stderr, "where <height> and <width> are the dimensions of the image.\n");
     return -1;
   }

   /*Another coding attempt 2*/
   //Initialize MPI
   MPI_Init(&argc,&argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);
   printf("I am %d of %d \n" , rank, size);

   //Start the timing
   t_start = MPI_Wtime (); /* Start timer */

   string name = "results_";
   string num = to_string(size);
   string ext = ".dat";
   string full_name = name + num + ext ;
   //Open the file
   if (rank == 0) {
     fp = fopen(full_name.c_str(), "w");
     assert (fp != NULL);
   }



   //Declaration of variables
   int i, j, n , N, num_proc, row_count;
   double *rec_buff;
   double * send_buf;
   int send_buf_row_count;
   double it, jt, x, y;

   //Initialize variables
   num_proc = rank;
   N = floor(height/size);
   row_count = 0;
   send_buf_row_count = 0;
   it = (maxY - minY)/height;
   jt = (maxX - minX)/width;

   //Determine the size of the array that each process handles
   // for (i = num_proc * N, n = 0; i < height && n < N; n++, i = num_proc * N + n){
   //   row_count +=1 ;
   // }
   row_count = N;

   // call mandelbrot computation on each of the arrays
   send_buf = (double *)malloc(row_count * width * sizeof(double));
   cout << N;
   for (i = num_proc * N, n = 0; i < height && n < N;n++, i = num_proc * N + n){
     x = minX;
     y = i * it + minY;
     for (int j = 0; j < width; ++j) {
       send_buf[(send_buf_row_count * width) + j] = mandelbrot(x,y)/512.0;
       x += jt;
     }
     send_buf_row_count += 1;
   }
   // Root process creates rec_buff
   if(rank == 0){
     // for (int k = 0 ; k < height; k++){
       rec_buff = (double *)malloc(height * width * sizeof(double));
     // }
   }


   //Call Gather for each of the arrays of varying sizes
   MPI_Gather(send_buf, row_count * width, MPI_DOUBLE,rec_buff, row_count * width, MPI_DOUBLE, 0, MPI_COMM_WORLD);
   if(rank == 0){
     printf("Mandelbrot computation right after MPI_gather : %.10f", MPI_Wtime () - t_start);
    }

   if(rank > 0){
     free(send_buf);
   }

   //print contents of rec_buff
   if(rank < 0){
     for (i = 0 ; i < height ; i ++){
       for (j = 0 ; j < width ; j++){
         if(rec_buff[i * width + j] > 0){
          cout << rec_buff[i * width + j] << ", " ;
        }
       }
     }
   }

   int row_begin_index = 0;
   int r, p,max_row,pixel;
   //only root render
   if(rank == 0){
     //create the image array
     gil::rgb8_image_t img(height, width);
     auto img_view = gil::view(img);
     //go through each process within the array
     for (p = 0 ; p < size ; p++){
       max_row = 0;
       max_row = N; //i think this also works
       // for (i = num_proc * N, n = 0; i < height && n < N; n++, i = num_proc * N + n){
       //   max_row += 1;
       // }
       for(r = 0 ; r < max_row ; r++){
         for (pixel = 0 ; pixel < width ; pixel++){
           img_view(pixel, p * N + r) = render(rec_buff[row_begin_index + r * width + pixel]);
         }
       }
       row_begin_index += max_row * width;
     }
     gil::png_write_view("mandelbrot_joe_test.png", const_view(img));
   }

   MPI_Barrier(MPI_COMM_WORLD);
   //Close the file
   if (rank == 0) {
     t_elapsed = MPI_Wtime () - t_start; /* Stop timer */
     //Write result to the file
     fprintf (fp, "%d\t%.10f\n", size, t_elapsed);
     fflush (fp);
     fclose (fp); /* Close results.dat */
   }


   MPI_Finalize();

}


 /* eof */
