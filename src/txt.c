#include "serverHTTP.h"


void main(int argc,char **argv){
	if(argc != 4) printf("manque des arguments\n");
	seuil = atoi(argv[3]);
	start(atoi(argv[1]),atoi(argv[2]));
	loopS();
}
