#include "mulhttp.h"

int main(int argc, char **argv)
{
	int ret = 0;
	
	mulhttp_t *mulhttp = init_mulhttp(argv[1]);
	
	start_download(mulhttp);
	
	wait_content(mulhttp);
	
	write_file(mulhttp, "/data/local/ss.apk");
	
	while(1){
		
	}
	
	return ret;
}