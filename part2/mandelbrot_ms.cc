/**
 *  \file mandelbrot_ms.cc
 *
 *  \brief Implement your parallel mandelbrot set in this file.
 */
/*
  Author : Diego Torres
  Date Modified : 16 May 2019
  Description : Master-Slave implementation of mandelbrot

  qsub mandelbrot_ms_1.sh ;qsub mandelbrot_ms_2.sh ;qsub mandelbrot_ms_4;qsub mandelbrot_ms_8.sh ;qsub mandelbrot_16.sh ;qsub mandelbrot_ms_32.sh ;qsub mandelbrot_ms_64.sh
qsub mandelbrot_joe_1.sh;qsub mandelbrot_joe_2.sh;qsub mandelbrot_joe_4.sh;qsub mandelbrot_joe_8.sh;qsub mandelbrot_joe_16.sh;qsub mandelbrot_joe_32.sh;qsub mandelbrot_joe_64.sh 

*/
#include <iostream>
#include <cstdlib>
#include <mpi.h>
#include <cstring>
#include <string.h>
#include "render.hh"

 using namespace std;
 #define WIDTH 1000
 #define HEIGHT 1000

// mandelbrot computation function
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

int main (int argc, char* argv[])
{
   /*
    Declare Variables
   */
   MPI_Status status;
   double * row_array;
   double it, jt, x, y;
   int width, height;
   int rank, size, al, i, j, msg, row,col,process,num_slaves;
   string name, num, ext, full_name;
   double minX = -2.1;
   double maxX = 0.7;
   double minY = -1.25;
   double maxY = 1.25;
   FILE * fp = NULL;
   double t_start, t_elapsed;

   /*
    Parse through supplied arguments
   */
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


  /* Lucky you, you get to write MPI code */
  //Initialize MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  printf("I am %d of %d \n" , rank, size);

  // Initialize variables for the following code
  name = "mandelbrot_ms_";
  num = to_string(size);
  ext = ".dat";
  full_name = name + num + ext;
  it = (maxY - minY)/height;
  jt = (maxX - minX)/width;
  double *mandelbrot_2D[height];
  num_slaves = size - 1;
  row_array = (double *)malloc(width * sizeof(double));

  //Open the file
  if (rank == 0) {
    fp = fopen(full_name.c_str(), "w");
    assert (fp != NULL);
  }

  //Start the timing
  t_start = MPI_Wtime (); /* Start timer */


  // Master Code
  if(rank == 0){
    // cout<<"beginning of the master code"<<endl;
    //allocate space for mandelbrot
    for(al = 0 ; al < height ; al++){
      mandelbrot_2D[al] = (double *)malloc(width * sizeof(double));
    }
    cout <<"allocating to the mandelbrot array"<<endl;
    // Send the initial messages to the slaves to num_slaves
    for (i = 0 ; i < num_slaves; i++){
      //send
      MPI_Send(&i, 1, MPI_INT, i+1, 0, MPI_COMM_WORLD);
    }
    cout<<"sending initial messages to the slave"<<endl;
    // Continue sending the rest of the messages as they are recieved from each process
    for (i = num_slaves; i < height + num_slaves; i++){
      // Recieve
      MPI_Recv(row_array, width, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      row = status.MPI_TAG;
      process = status.MPI_SOURCE;

      // save mandelbrot
      memcpy(mandelbrot_2D[row], row_array, width * sizeof(double));

      // terminating sequence
      if ((i)<height){
        msg=i;
      }
      else{
        msg=-1;
      }

      // data, count, datatype, destination, tag, communicator
      // send what row to compute
      //MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
      MPI_Send(&msg,1,MPI_INT, process, 0, MPI_COMM_WORLD);
    }
    // cout<<"end of the master code"<<endl;
  }
  // Slave Code
  else{
    // cout<<"beginning of the slave code"<<endl;
    while(1){
      MPI_Recv(&row, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

      //terminate the slave code
      if(row == -1){
        break;
      }
      x = minX;
      y = row * it + minY;
      for(j = 0 ; j < width; j++){
        row_array[j] = mandelbrot(x,y)/512.0;
        x+= jt;
      }

      MPI_Send(row_array,width,MPI_DOUBLE,0,row,MPI_COMM_WORLD);
    }
    // cout<<"end of the slave code"<<endl;
  }
  printf ("%Time for MASTER/SLAVE : d\t%.10f\n", size,MPI_Wtime () - t_start );

  // Render
  if(rank == 0){
    cout<<"beginning of render code"<<endl;
    //create the image array
    gil::rgb8_image_t img(height, width);
    auto img_view = gil::view(img);
    //do the rendering itself by iterating through every pixel in the image
    for(row = 0 ; row < height; row++){
      for(col = 0 ; col < width; col++){
        img_view(col, row) = render(mandelbrot_2D[row][col]);
      }
    }
    gil::png_write_view("mandelbrot_ms.png", const_view(img));
    printf ("%Time for RENDERING: d\t%.10f\n", size,MPI_Wtime () - t_start );
    cout<<"end of the render code"<<endl;

    //Timing and write to result to file
    if (rank == 0) {
      t_elapsed = MPI_Wtime () - t_start; /* Stop timer */
      //Write result to the file
      fprintf (fp, "%d\t%.10f\n", size, t_elapsed);
      fflush (fp);
      fclose (fp); /* Close results.dat */
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();

}

/* eof */
