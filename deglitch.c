/* 
 * deglitch
 * 
 * Adjust errored values (glitches) in image.
 */

#include <stdio.h>
#include <math.h>
#include <netpbm/pam.h>

int TopDown = 1;
int debug = 1;

void Usage(char *app_name) {
    fprintf(stderr,"Usage: %s [options] infile [outfile]\n", app_name);
    
    fprintf(stderr,"            infile1  :   Input file. Top (or left) image.\n");
    fprintf(stderr,"            outfile1 :   Output file. Deglitched image.\n");
    fprintf(stderr,"    Options:\n");
    fprintf(stderr,"            -h :  This usage message.\n");
}

void deglitch_sample(struct pam pam, tuple **t, unsigned r, unsigned c, unsigned p) {
    long res = 0;
    long v;
    int samples = 0;
    if ( r>0 ) {
        v = t[r-1][c][p];
        if ( v>0 && v<65500 ) {
            res += v;
            samples += 1;
        }
    }
    if ( r<pam.height-1 ) {
        v = t[r+1][c][p];
        if ( v>0 && v<65500 ) {
            res += v;
            samples += 1;
        }
    }
    if ( c>0 ) {
        v = t[r][c-1][p];
        if ( v>0 && v<65500 ) {
            res += v;
            samples += 1;
        }
    }
    if ( c<pam.width-1 ) {
        v = t[r][c+1][p];
        if ( v>0 && v<65500 ) {
            res += v;
            samples += 1;
        }
    }
    if ( samples==0 ) return;
    
    res /= samples;
    t[r][c][p] = res;
    
}

int main(int argc, char **argv) {
    struct pam inpam;
    struct pam outpam;

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
    FILE *fin = NULL;
    if ( argv[1] )
        fin = fopen(argv[1], "r");
    tuple **tuples = NULL;
    if ( fin ) {
        tuples = pnm_readpam(fin, &inpam, PAM_STRUCT_SIZE(tuple_type));
        pm_close(fin);
        fprintf(stderr,"Input file: %dx%dx%d\n", inpam.width, inpam.height, inpam.depth);
    }
    else {
        fprintf(stderr, "Cannot read image '%s'\n", argv[1]);
        exit(1);
    }

   // Count saturated (errored) samples in image
   int count=0;
   
   unsigned int row;
   for (row = 0; row < inpam.height; ++row) {
       unsigned int column;
       for (column = 0; column < inpam.width; ++column) {
           unsigned int plane;
           for (plane = 0; plane < inpam.depth; ++plane) {
               if ( tuples[row][column][plane]>60000 || tuples[row][column][plane]<1 ) {
                   count++;
                   if ( debug ) 
                       fprintf(stderr,"Over height sample in image %d %d %d: %ld\n", row, column, plane,
                           tuples[row][column][plane]);
               }
               deglitch_sample(inpam,tuples,row, column, plane);
           }
       }
   }
   fprintf(stderr,"Count: %d over heighted samples in image.\n", count);
   
   
   // correct column 1
   unsigned int plane;
   for (plane = 0; plane < inpam.depth; ++plane) {
        unsigned int column;
        long master_total = 0L;
        long totals[10];
        for (column = 0; column < inpam.width && column < 10 ; ++column) {
            long total = 0L;
            for (row = 0; row < inpam.height; ++row) {
                total += tuples[row][column][plane];
            }
            master_total += total;
            totals[column] = total;
            printf("Column %d, total %ld\n", column, total);
        }
        master_total -= totals[0];
        master_total -= totals[1];
        master_total -= totals[2];
        master_total -= totals[3];
        for (column = 0; column < inpam.width && column < 4 ; ++column) {
            long total = 0;
            for (row = 0; row < inpam.height ; ++row) {
                tuples[row][column][plane] -= (totals[column]-master_total/6)/inpam.height;
                total += tuples[row][column][plane];
                long delta = tuples[row][column][plane] - tuples[row][column+1][plane];
                for( int c=column ; c>=0 && abs(delta)>10 ; c-- ) {
                    if ( delta>0. )
                        tuples[row][c][plane] = tuples[row][c+1][plane] + 10;
                    else
                        tuples[row][c][plane] = tuples[row][c+1][plane] - 10;
                    if ( c==0 ) break;
                    delta = tuples[row][c-1][plane] - tuples[row][c][plane];
                }
            }
            printf("Column %d, total %ld -> %ld\n", column, totals[column], total);
        }
        //tuples[1000][3][plane] = 60000;
   }
   
   
   
   
   
   
   // Verify declive
   count = 0;
   for (row = 0; row < inpam.height; ++row) {
       unsigned int column;
       for (column = 0; column < inpam.width; ++column) {
           unsigned int plane;
           for (plane = 0; plane < inpam.depth; ++plane) {
               if ( row>0 && abs(tuples[row][column][plane]-tuples[row-1][column][plane]) > 100 ) {
                   count++;
                   //fprintf(stderr,"Falesia row %d %d %d: %ld %ld\n", row, column, plane,
                     //      tuples[row][column][plane], tuples[row-1][column][plane]);
               }
               if ( column>0 && abs(tuples[row][column][plane]-tuples[row][column-1][plane]) > 100 ) {
                   count++;
                   //fprintf(stderr,"Falesia column %d %d %d: %ld %ld\n", row, column, plane,
                           //tuples[row][column][plane], tuples[row][column-1][plane]);
               }
               
           }
       }
   }
   fprintf(stderr,"Count: %d large declives.\n", count);
 

    // Write out image 
    FILE *fout = fopen(argv[2],"w");
    if ( fout ) {
        outpam = inpam; outpam.file = fout;
        pnm_writepam( &outpam, tuples);
        pm_close( fout );
    }
    
    pnm_freepamarray(tuples, &inpam);
}
