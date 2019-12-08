   /* Example program fragment to read a PAM or PNM image
      from stdin, add up the values of every sample in it
      (I don't know why), and write the image unchanged to
      stdout. */

#include <stdio.h>
#include <math.h>
#include <netpbm/pam.h>

int grand_total = 0;

int main(int argc, char **argv) {
    struct pam inpam1;
    struct pam inpam2;
    struct pam outpam1;
    struct pam outpam2;
   
    unsigned int row;

    pm_init(argv[0], 0);
   
    tuple **tuples1 = NULL;
    FILE *fin1 = fopen(argv[1], "r");
    if ( fin1 ) {
        tuples1 = pnm_readpam(fin1, &inpam1, PAM_STRUCT_SIZE(tuple_type));
        pm_close(fin1);
        fprintf(stderr,"Input file1: %dx%dx%d\n", inpam1.width, inpam1.height, inpam1.depth);
    }
    
    tuple **tuples2 = NULL;
    FILE *fin2 = fopen(argv[2], "r");
    if ( fin2 ) {
        tuples2 = pnm_readpam(fin2, &inpam2, PAM_STRUCT_SIZE(tuple_type));
        pm_close(fin2);
        fprintf(stderr,"Input file2: %dx%dx%d\n", inpam2.width, inpam2.height, inpam2.depth);
    }

   // Count saturated (errored) samples in images
   int count1=0;
   int count2=0;
   for (row = 0; row < inpam1.height; ++row) {
       unsigned int column;
       for (column = 0; column < inpam1.width; ++column) {
           unsigned int plane;
           for (plane = 0; plane < inpam1.depth; ++plane) {
               if ( tuples1[row][column][plane]>60000 || tuples1[row][column][plane]<1 ) {
                   count1++;
               }
               if ( tuples2[row][column][plane]>60000 || tuples2[row][column][plane]<1 ) count2++;
           }
       }
   }
   fprintf(stderr,"Count: %d over heighted samples in image 1.\n", count1);
   fprintf(stderr,"Count: %d over heighted samples in image 2.\n", count2);
   
   // Evaluate joint differences
   unsigned int column;
   unsigned long diff = 0L; 
   for (column = 0; column < inpam1.width; ++column) {
        unsigned int plane;
        for (plane = 0; plane < inpam1.depth; ++plane) {
            diff += abs(tuples1[0][column][plane] - tuples2[inpam2.height-1][column][plane]);
        }
   }
   fprintf(stderr,"Diff: %ld \n", diff);
   
   count1 = count2 = 0;
   // Correct Images
   for (column = 0; column < inpam1.width; ++column) {
        unsigned int plane;
        for (plane = 0; plane < inpam1.depth; ++plane) {
/*
            fprintf(stderr,"%d,%d: Delta %d: (%lu %lu)\n", column, plane, delta,
                    tuples1[0][column][plane] , tuples2[inpam2.height-1][column][plane] );
                    */
//             
            if ( tuples1[0][column][plane]>60000 || tuples1[0][column][plane]<1 ) {
                tuples1[0][column][plane] = tuples2[inpam2.height-1][column][plane] ;
                count1++;
            }
            
            if ( tuples2[inpam2.height-1][column][plane]>60000 || tuples1[inpam2.height-1][column][plane]<1 ) {
                tuples1[inpam2.height-1][column][plane] = tuples1[0][column][plane] ;
                count2++;
            }
            
            int delta = tuples1[0][column][plane] - tuples2[inpam2.height-1][column][plane];
            float ddelta = (float)delta / (float)inpam1.height / 2.0;
            int d = delta/2;
            
            for (row = 0 ; row<inpam1.height / 2 && d!=0 ; row++ ) {
                d = round((float)delta/2. - ddelta*row) ;
                tuples1[row][column][plane] += d;
                tuples2[inpam2.height-1-row][column][plane] -= d;
            }
            int new_delta = 
                (unsigned short)(tuples1[0][column][plane]) - (unsigned short)(tuples2[inpam2.height-1][column][plane]);
                
            if ( abs(new_delta)>0 )
                fprintf(stderr,"%d,%d: Delta %d -> %d: %ld %ld (%lu %lu)\n", column, plane, delta, new_delta,
                    tuples1[0][column][plane] , tuples2[inpam2.height-1][column][plane],
                    tuples1[0][column][plane] , tuples2[inpam2.height-1][column][plane] );
        }
   }
   
        fprintf(stderr, "Border saturated samples corrected: %d %d\n", count1, count2); 

    // Write out image 1
    FILE *fout1 = fopen(argv[3],"w");
    if ( fout1 ) {
        outpam1 = inpam1; outpam1.file = fout1;
        //pnm_writepaminit(&outpam1);
        pnm_writepam( &outpam1, tuples1);
        pm_close( fout1 );
    }
    
    // Write out image 1
    FILE *fout2 = fopen(argv[4],"w");
    if ( fout2 ) {
        outpam2 = inpam2; outpam2.file = fout2;
        //pnm_writepaminit(&outpam1);
        pnm_writepam( &outpam2, tuples2);
        pm_close( fout2 );
    }
    
    pnm_freepamarray(tuples1, &inpam1);
    pnm_freepamarray(tuples2, &inpam2);
}
