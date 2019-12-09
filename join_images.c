/* 
 * join_images
 * 
 * Adjust values in image border so that the two images can be connected.
 * Adjusted images can be connected verticaly or side by side.
 */

#include <stdio.h>
#include <math.h>
#include <netpbm/pam.h>

int TopDown = 1;
int debug = 1;

void Usage(char *app_name) {
    fprintf(stderr,"Usage: %s [options] infile1 infile2 [outfile1 [outfile2]]\n", app_name);
    
    fprintf(stderr,"            infile1  :   Input file 1. Top (or left) image.\n");
    fprintf(stderr,"            infile2  :   Input file 2. Bottom (or right) image.\n");
    fprintf(stderr,"            outfile1 :   Output file 1. Adjusted top (or left) image.\n");
    fprintf(stderr,"            outfile2 :   Output file 2. Adjusted bottom (or right) image.\n");
    fprintf(stderr,"    Options:\n");
    fprintf(stderr,"            -h :  This usage message.\n");
    fprintf(stderr,"            -s :  Side-by-side method.\n\n");
}

int main(int argc, char **argv) {
    struct pam inpam1;
    struct pam inpam2;
    struct pam outpam1;
    struct pam outpam2;

    pm_init(argv[0], 0);
    char *app_name = argv[0];
    while( argc>1 && argv[1][0]=='-' ) {
        switch ( argv[1][1] ) {
            case 'h':
                Usage(app_name);
                exit(1);
            case 's':
                TopDown = 0;
                break;
        }
        argc--;
        argv++;
    }
    printf("TopDown: %d\n", TopDown);
    FILE *fin1 = NULL;
    if ( argv[1] )
        fin1 = fopen(argv[1], "r");
    tuple **tuples1 = NULL;
    if ( fin1 ) {
        tuples1 = pnm_readpam(fin1, &inpam1, PAM_STRUCT_SIZE(tuple_type));
        pm_close(fin1);
        fprintf(stderr,"Input file1: %dx%dx%d\n", inpam1.width, inpam1.height, inpam1.depth);
    }
    
    FILE *fin2 = NULL;
    if ( argv[2] )
        fin2 = fopen(argv[2], "r");
    tuple **tuples2 = NULL;
    if ( fin2 ) {
        tuples2 = pnm_readpam(fin2, &inpam2, PAM_STRUCT_SIZE(tuple_type));
        pm_close(fin2);
        fprintf(stderr,"Input file2: %dx%dx%d\n", inpam2.width, inpam2.height, inpam2.depth);
    }

   // Count saturated (errored) samples in images
   int count1=0;
   int count2=0;
   
   unsigned int row;
   for (row = 0; row < inpam1.height; ++row) {
       unsigned int column;
       for (column = 0; column < inpam1.width; ++column) {
           unsigned int plane;
           for (plane = 0; plane < inpam1.depth; ++plane) {
               if ( tuples1[row][column][plane]>60000 || tuples1[row][column][plane]<1 ) {
                   count1++;
                   if ( debug ) 
                       fprintf(stderr,"Over height sample in image1 %d %d %d: %ld\n", row, column, plane,
                           tuples1[row][column][plane]);
               }
               if ( tuples2[row][column][plane]>60000 || tuples2[row][column][plane]<1 ) {
                   count2++;
                   if ( debug ) 
                       fprintf(stderr,"Over height sample in image2 %d %d %d: %ld\n", row, column, plane, 
                           tuples2[row][column][plane]);
               }
           }
       }
   }
   fprintf(stderr,"Count: %d over heighted samples in image 1.\n", count1);
   fprintf(stderr,"Count: %d over heighted samples in image 2.\n", count2);
   
    // Evaluate joint differences
    unsigned long diff = 0L; 
    
    unsigned int plane;
    for (plane = 0; plane < inpam1.depth; ++plane) {
        if ( TopDown ) {
            unsigned int column;
            for (column = 0; column < inpam1.width; ++column) 
                diff += abs(tuples1[inpam1.height-1][column][plane] - tuples2[0][column][plane]);
        }
        else {      // Left to Right
            unsigned int row;
            for (row = 0; row < inpam1.height; ++row) {
                diff += abs(tuples1[row][inpam1.width-1][plane] - tuples2[row][0][plane]);
            }
        }
    }
    
    fprintf(stderr,"Diff: %ld \n", diff);
    if ( diff > 200000L ) {
        fprintf(stderr, "Too large joint error. Exiting\n");
        exit(1);
    }
   
    count1 = count2 = 0;
    // Correct Images
   
    for (plane = 0; plane < inpam1.depth; ++plane) {
        if ( TopDown) {
            unsigned int column;
            for (column = 0; column < inpam1.width; ++column) {
                if ( tuples1[inpam1.height-1][column][plane]>60000 || tuples1[inpam1.height-1][column][plane]<1 ) {
                    tuples1[inpam1.height-1][column][plane] = tuples2[0][column][plane] ;
                    count1++;
                }           
                if ( tuples2[0][column][plane]>60000 || tuples2[0][column][plane]<1 ) {
                    tuples2[0][column][plane] = tuples1[inpam1.height-1][column][plane] ;
                    count2++;
                }
                    
                int delta = tuples1[inpam1.height-1][column][plane] - tuples2[0][column][plane];
                float ddelta = (float)delta / (float)inpam1.height / 2.0;
                int d = delta/2;

                for (row = 0 ; row<inpam1.height / 2 && d!=0 ; row++ ) {
                    d = round((float)delta/2. - ddelta*row) ;
                    tuples1[inpam1.height-1-row][column][plane] += d;
                    tuples2[                row][column][plane] -= d;
                }
                int new_delta = (unsigned short)(tuples1[inpam1.height-1][column][plane]) 
                            - (unsigned short)(tuples2[0][column][plane]);
            
                if ( abs(new_delta)>0 )
                    fprintf(stderr,"%d,%d: Delta %d -> %d\n", column, plane, delta, new_delta);
            }
        }
        else {
            unsigned int row;
            for (row = 0; row < inpam1.height ; ++row) {
                if ( tuples1[row][inpam1.width-1][plane]>60000 || tuples1[row][inpam1.width-1][plane]<1 ) {
                    tuples1[row][inpam1.width-1][plane] = tuples2[row][0][plane] ;
                    count1++;
                }           
                if ( tuples2[row][0][plane]>60000 || tuples2[row][0][plane]<1 ) {
                    tuples2[row][0][plane] = tuples1[row][inpam1.width-1][plane] ;
                    count2++;
                }
                    
                int delta = tuples1[row][inpam1.width-1][plane] - tuples2[row][0][plane];
                float ddelta = (float)delta / (float)inpam1.width / 2.0;
                int d = delta/2;
                
                int column;
                for (column = 0 ; column<inpam1.width / 2 && d!=0 ; column++ ) {
                    d = round((float)delta/2. - ddelta*column) ;
                    tuples1[row][inpam1.width-1-column][plane] += d;
                    tuples2[row][               column][plane] -= d;
                }
                int new_delta = (unsigned short)(tuples1[row][inpam1.width-1][plane]) 
                              - (unsigned short)(tuples2[row][0][plane]);
            
                if ( abs(new_delta)>0 )
                    fprintf(stderr,"%d,%d: Delta %d -> %d\n", row, plane, delta, new_delta);
            }
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
