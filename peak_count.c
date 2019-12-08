   /* Example program fragment to read a PAM or PNM image
      from stdin, add up the values of every sample in it
      (I don't know why), and write the image unchanged to
      stdout. */

#include <stdio.h>
#include <netpbm/pam.h>

int grand_total = 0;



int main(int argc, char **argv) {
   struct pam inpam, outpam;
   tuple * tuplerow;
   unsigned int row;

   pm_init(argv[0], 0);

   pnm_readpaminit(stdin, &inpam, PAM_STRUCT_SIZE(tuple_type));

   fprintf(stderr,"Input file: %dx%dx%d\n", inpam.width, inpam.height, inpam.depth);
   
   outpam = inpam; outpam.file = stdout;

   pnm_writepaminit(&outpam);

   tuplerow = pnm_allocpamrow(&inpam);
   int count=0;
   for (row = 0; row < inpam.height; ++row) {
       unsigned int column;
       pnm_readpamrow(&inpam, tuplerow);
       for (column = 0; column < inpam.width; ++column) {
           unsigned int plane;
           for (plane = 0; plane < inpam.depth; ++plane) {
               grand_total += tuplerow[column][plane];
                if ( tuplerow[column][plane]>60000 ) count++ ;
           }
       }
       pnm_writepamrow(&outpam, tuplerow);
   }
   pnm_freepamrow(tuplerow);
   
   fprintf(stderr,"Count: %d over heighted samples\n", count);
}
