#include <stdio.h>
#include <string.h>
#define R_OK 4
int main() {
  FILE *fp;
  char s1[50],s2[50];
  int version,i,j ;
  i = access(".svn/entries",R_OK);
  if (i < 0) {
    return 0;
  }
  else {
    fp = popen("svn info| grep Revision ","r");
    fscanf(fp,"%s%d",s1,&version);
    printf("%i\n",version);
  }
  return 0;
}
