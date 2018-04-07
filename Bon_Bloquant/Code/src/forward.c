#include <stdio.h>
#include <math.h>
#include <shalw.h>
#include <export.h>
#include <mpi.h>
#include <stdlib.h>


#define TAG_HFIL 1
#define TAG_UFIL 2
#define TAG_VFIL 3
#define TAG_HPHY 4
#define TAG_UPHY 5
#define TAG_VPHY 6


double hFil_forward(int t, int i, int j) {
  //Phase d'initialisation du filtre
  //HPHY(t - 1, i, j) est encore nul
  if (t <= 2)
    return HPHY(t, i, j);
  return HPHY(t - 1, i, j) +
    alpha * (HFIL(t - 1, i, j) - 2 * HPHY(t - 1, i, j) + HPHY(t, i, j));
}

double uFil_forward(int t, int i, int j) {
  //Phase d'initialisation du filtre
  //UPHY(t - 1, i, j) est encore nul
  if (t <= 2)
    return UPHY(t, i, j);
  return UPHY(t - 1, i, j) +
    alpha * (UFIL(t - 1, i, j) - 2 * UPHY(t - 1, i, j) + UPHY(t, i, j));
}

double vFil_forward(int t, int i, int j) {
  //Phase d'initialisation du filtre
  //VPHY(t - 1, i, j) est encore nul
  if (t <= 2)
    return VPHY(t, i, j);
  return VPHY(t - 1, i, j) +
    alpha * (VFIL(t - 1, i, j) - 2 * VPHY(t - 1, i, j) + VPHY(t, i, j));
}

double hPhy_forward(int t, int i, int j) {
  double c, d;
  
  c = 0.;
  if (i > 0)
    c = UPHY(t - 1, i - 1, j);

  d = 0.;
  if (j < size_y - 1)
    d = VPHY(t - 1, i, j + 1);

  return HFIL(t - 1, i, j) -
    dt * hmoy * ((UPHY(t - 1, i, j) - c) / dx +
     (d - VPHY(t - 1, i, j)) / dy);
}

double uPhy_forward(int t, int i, int j) {
  double b, e, f, g;
  
  if (i == size_x - 1)
    return 0.;

  b = 0.;
  if (i < size_x - 1)
    b = HPHY(t - 1, i + 1, j);

  e = 0.;
  if (j < size_y - 1)
    e = VPHY(t - 1, i, j + 1);

  f = 0.;
  if (i < size_x - 1)
    f = VPHY(t - 1, i + 1, j);

  g = 0.;
  if (i < size_x - 1 && j < size_y - 1)
    g = VPHY(t - 1, i + 1, j + 1);

  return UFIL(t - 1, i, j) +
    dt * ((-grav / dx) * (b - HPHY(t - 1, i, j)) +
    (pcor / 4.) * (VPHY(t - 1, i, j) + e + f + g) -
    (dissip * UFIL(t - 1, i, j)));
}

double vPhy_forward(int t, int i, int j) {
  double c, d, e, f;

  if (j == 0)
    return 0.;

  c = 0.;
  if (j > 0)
    c = HPHY(t - 1, i, j - 1);

  d = 0.;
  if (i > 0 && j > 0)
    d = UPHY(t - 1, i -1, j -1);

  e = 0.;
  if (i > 0)
    e = UPHY(t - 1, i - 1, j);

  f = 0.;
  if (j > 0)
    f = UPHY(t - 1, i, j - 1);

  return VFIL(t - 1, i, j) +
    dt * ((-grav / dy) * (HPHY(t - 1, i, j) - c) -
    (pcor / 4.) * (d + e + f + UPHY(t - 1, i, j)) -
    (dissip * VFIL(t - 1, i, j)));
}

void forward(void) {
   FILE *file = NULL;
   double svdt = 0.;
   int t = 0;
   MPI_Status status;
   int local_size_x,local_size_y;

  if (file_export && my_rank == 0) 
   {
      file = create_file();
      export_step(file, t);
   }
   /* Taille de bloc provisoir  */
   int nb_bloc = size_x/np;

   /* Initialisation des variables locales */
  local_size_x = (my_rank==0 || my_rank==np-1) ? (nb_bloc+1) : (nb_bloc+2);
  // printf("local_size_x = %d sur le process %d\n",local_size_x,my_rank);

  local_size_y = size_y;

  // MPI_Bcast(&nb_steps,1,MPI_INT,0,MPI_COMM_WORLD);

  // if(my_rank==0)
  // {
  //   HFIL(0,64,0) = 199;
  //   HFIL(0,128,0) = 1990;
  // }
  
  // MPI_Scatter(&HFIL(0, 0, 0),nb_bloc*size_y,MPI_DOUBLE,&HFIL(0,0,0)+my_rank*nb_bloc*local_size_y,nb_bloc*size_y,MPI_DOUBLE,0,MPI_COMM_WORLD);

  // HFIL(0,148,200) = 111111.0;

   for (t = 1; t < nb_steps; t++) 
   {  
      
      if (t == 1) 
      {
    	 svdt = dt;
    	 dt = 0;
      }
      if (t == 2)
      {
	     dt = svdt / 2.;
      }
  
      for (int j = 0; j < local_size_y; j++) 
      {
        // printf("Derniere ligne de trou %lf sur le process %d\n",HPHY(1,local_size_x-1,j),my_rank);
        // printf("Deuxieme ligne %lf sur le process %d\n",HPHY(1,1,j),my_rank);
        // if(j==10) break;

        for (int i = my_rank*nb_bloc; i < (my_rank+1)*nb_bloc; i++) 
       {   

          HPHY(t, i, j) = hPhy_forward(t, i, j);
          UPHY(t, i, j) = uPhy_forward(t, i, j);
          VPHY(t, i, j) = vPhy_forward(t, i, j);
          HFIL(t, i, j) = hFil_forward(t, i, j);
          UFIL(t, i, j) = uFil_forward(t, i, j);
          VFIL(t, i, j) = vFil_forward(t, i, j);

       }
      }
       if(my_rank!=np-1)
      {
        MPI_Sendrecv(&HPHY(t,0,0)+((local_size_y*((my_rank+1)*nb_bloc-1))),local_size_y,MPI_DOUBLE,my_rank+1,TAG_HPHY,&HPHY(t,0,0)+local_size_y*(my_rank+1)*nb_bloc,local_size_y,MPI_DOUBLE,my_rank+1,TAG_HPHY,MPI_COMM_WORLD,&status);
        MPI_Sendrecv(&UPHY(t,0,0)+((local_size_y*((my_rank+1)*nb_bloc-1))),local_size_y,MPI_DOUBLE,my_rank+1,TAG_UPHY,&UPHY(t,0,0)+local_size_y*(my_rank+1)*nb_bloc,local_size_y,MPI_DOUBLE,my_rank+1,TAG_UPHY,MPI_COMM_WORLD,&status);
        MPI_Sendrecv(&VPHY(t,0,0)+((local_size_y*((my_rank+1)*nb_bloc-1))),local_size_y,MPI_DOUBLE,my_rank+1,TAG_VPHY,&VPHY(t,0,0)+local_size_y*(my_rank+1)*nb_bloc,local_size_y,MPI_DOUBLE,my_rank+1,TAG_VPHY,MPI_COMM_WORLD,&status);
        MPI_Sendrecv(&HFIL(t,0,0)+((local_size_y*((my_rank+1)*nb_bloc-1))),local_size_y,MPI_DOUBLE,my_rank+1,TAG_HFIL,&HFIL(t,0,0)+local_size_y*(my_rank+1)*nb_bloc,local_size_y,MPI_DOUBLE,my_rank+1,TAG_HFIL,MPI_COMM_WORLD,&status);
        MPI_Sendrecv(&UFIL(t,0,0)+((local_size_y*((my_rank+1)*nb_bloc-1))),local_size_y,MPI_DOUBLE,my_rank+1,TAG_UFIL,&UFIL(t,0,0)+local_size_y*(my_rank+1)*nb_bloc,local_size_y,MPI_DOUBLE,my_rank+1,TAG_UFIL,MPI_COMM_WORLD,&status);
        MPI_Sendrecv(&VFIL(t,0,0)+((local_size_y*((my_rank+1)*nb_bloc-1))),local_size_y,MPI_DOUBLE,my_rank+1,TAG_VFIL,&VFIL(t,0,0)+local_size_y*(my_rank+1)*nb_bloc,local_size_y,MPI_DOUBLE,my_rank+1,TAG_VFIL,MPI_COMM_WORLD,&status);
      }
     if(my_rank!=0)
      {
        MPI_Sendrecv(&HPHY(t,0,0)+my_rank*nb_bloc*local_size_y,local_size_y,MPI_DOUBLE,my_rank-1,TAG_HPHY,&HPHY(t,0,0)+(my_rank*nb_bloc-1)*local_size_y,local_size_y,MPI_DOUBLE,my_rank-1,TAG_HPHY,MPI_COMM_WORLD,&status);
        MPI_Sendrecv(&UPHY(t,0,0)+my_rank*nb_bloc*local_size_y,local_size_y,MPI_DOUBLE,my_rank-1,TAG_UPHY,&UPHY(t,0,0)+(my_rank*nb_bloc-1)*local_size_y,local_size_y,MPI_DOUBLE,my_rank-1,TAG_UPHY,MPI_COMM_WORLD,&status);
        MPI_Sendrecv(&VPHY(t,0,0)+my_rank*nb_bloc*local_size_y,local_size_y,MPI_DOUBLE,my_rank-1,TAG_VPHY,&VPHY(t,0,0)+(my_rank*nb_bloc-1)*local_size_y,local_size_y,MPI_DOUBLE,my_rank-1,TAG_VPHY,MPI_COMM_WORLD,&status);/*PROBLEME SUR VPHY*/
        MPI_Sendrecv(&HFIL(t,0,0)+my_rank*nb_bloc*local_size_y,local_size_y,MPI_DOUBLE,my_rank-1,TAG_HFIL,&HFIL(t,0,0)+(my_rank*nb_bloc-1)*local_size_y,local_size_y,MPI_DOUBLE,my_rank-1,TAG_HFIL,MPI_COMM_WORLD,&status);
        MPI_Sendrecv(&UFIL(t,0,0)+my_rank*nb_bloc*local_size_y,local_size_y,MPI_DOUBLE,my_rank-1,TAG_UFIL,&UFIL(t,0,0)+(my_rank*nb_bloc-1)*local_size_y,local_size_y,MPI_DOUBLE,my_rank-1,TAG_UFIL,MPI_COMM_WORLD,&status);
        MPI_Sendrecv(&VFIL(t,0,0)+my_rank*nb_bloc*local_size_y,local_size_y,MPI_DOUBLE,my_rank-1,TAG_VFIL,&VFIL(t,0,0)+(my_rank*nb_bloc-1)*local_size_y,local_size_y,MPI_DOUBLE,my_rank-1,TAG_VFIL,MPI_COMM_WORLD,&status);
      }

      MPI_Gather(&HFIL(t,0,0)+local_size_y*(my_rank*nb_bloc),nb_bloc*local_size_y,MPI_DOUBLE,&HFIL(t,0,0),nb_bloc*local_size_y,MPI_DOUBLE,0,MPI_COMM_WORLD);

        // FAIRE UN PETIT GATHER ICI
        // MPI_Gather(&HFIL(t,0,0)+local_size_y*(my_rank!=0),nb_bloc*local_size_y,MPI_DOUBLE,&HFIL(0,0,0),nb_bloc*local_size_y,MPI_DOUBLE,0,MPI_COMM_WORLD);
        // MPI_Gather(&HFIL(1,0,0)+local_size_y*(my_rank!=0),nb_bloc*local_size_y,MPI_DOUBLE,&HFIL(1,0,0),nb_bloc*local_size_y,MPI_DOUBLE,0,MPI_COMM_WORLD);
      // MPI_Barrier(MPI_COMM_WORLD);
        if (file_export && my_rank==0) 
        {
  	     export_step(file, t);
        }
      
        if (t == 2) 
        {
  	 dt = svdt;
        }
   }

   if (file_export&&my_rank==0) 
   {
      finalize_export(file);
   }
}
