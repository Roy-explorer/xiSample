// xiSample.cpp : program that captures 10 images
// sudo tee /sys/module/usbcore/parameters/usbfs_memory_mb >/dev/null <<<0
#include "stdafx.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#ifdef WIN32
#include "xiApi.h"       // Windows
#else
#include <m3api/xiApi.h> // Linux, OSX
#endif
#include <memory.h>

#define HandleResult(res,place) if (res!=XI_OK) {printf("Error after %s (%d)\n",place,res);goto finish;}

#include <termios.h>
#include <fcntl.h>
 
int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;
 
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
  ch = getchar();
 
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
 
  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }
 
  return 0;
}
int _tmain(int argc, _TCHAR* argv[])
{
	// image buffer
	XI_IMG image;
	memset(&image, 0, sizeof(image));
	image.size = sizeof(XI_IMG);
	
	int images = 0;
	char filename[256] = {0};
	
	FILE * pFile;
	char foldername[256] = {0};
	
	FILE * pFile_time;
	char filename_time[256] = {0};
	
	HANDLE xiH = NULL;
	XI_RETURN stat = XI_OK;
	
	// Get system time
	time_t t = time(NULL);
	struct tm *p = localtime(&t);
	
	int test = 0;
	int exposuretime=5000;
	float gain_in_db = 6.0f;
	int exposure_time = 0;
	int gain = 0;
	int device_value = 0;
	int wfd = -1;
	// Retrieving a handle to the camera device 
	printf("Opening first camera...\n");
	stat = xiOpenDevice(0, &xiH);
	HandleResult(stat, "xiOpenDevice");

	// Setting "Auto exposure & gain set" parameter (0=off 1=on)
	stat = xiSetParamInt(xiH, XI_PRM_AEAG, 1);
	HandleResult(stat, "xiSetParam (Auto exposure & gain set)");
	
	
	// Setting "Automatic exposure/gain set" parameter (0 ~ 1)
	stat = xiSetParamInt(xiH, XI_PRM_EXP_PRIORITY, 1);
	HandleResult(stat, "xiSetParam (Automatic exposure/gain set)");
	
	
	// Setting "exposure" parameter (us,1ms=1000us)
	//stat = xiSetParamInt(xiH, XI_PRM_EXPOSURE, exposuretime);
	//HandleResult(stat, "xiSetParam (exposure set)");
	
	// Setting "gain" parameter (-1.5 ~ 6 db)
	//stat = xiSetParamFloat(xiH, XI_PRM_GAIN, gain_in_db);
	//HandleResult(stat, "xiSetParam (gain set)");
	
	
	// Setting "data_format" parameter 
	stat = xiSetParamInt(xiH, XI_PRM_IMAGE_DATA_FORMAT, XI_MONO16);
	HandleResult(stat, "xiSetParam (data_Format set)");
	
	// Note:
	// The default parameters of each camera might be different in different API versions
	// In order to ensure that your application will have camera in expected state,
	// please set all parameters expected by your application to required value.
	
	printf("Starting acquisition...(%d, %d, %d)\n",p->tm_hour,p->tm_min,p->tm_sec);
	stat = xiStartAcquisition(xiH);
	HandleResult(stat, "xiStartAcquisition");
	
	time(&t);
	p = localtime(&t);
	
	// Make folder
	sprintf(foldername, "/media/pi/NIR/data/%d_%d_%d_%d_%d_%d", (1900+p->tm_year),(1+p->tm_mon),p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec);
	test = mkdir(foldername,0755);
	printf("%s_%d\n",foldername,test);
	//sleep(10);
	
	sprintf(filename_time, "%s%s", foldername, "/time.bin");
	pFile_time = fopen(filename_time, "wb");

	while(!kbhit())
	{
		images++;
		sprintf(filename, "%s/image_%d.bin", foldername, images);
		pFile = fopen(filename, "wb");
		stat = xiGetImage(xiH, 5000, &image);
		HandleResult(stat, "xiGetImage");
		unsigned char pixel = *(unsigned char*)image.bp;
		exposure_time = image.exposure_time_us;
		gain = (int)(image.gain_db);
		//printf("%d %f",exposure_time,gain);
		printf("Image %d (%dx%d) received from camera. First pixel value: %d,exposure: %d,gain: %f\n", images, (int)image.width, (int)image.height, pixel, image.exposure_time_us, (float)image.gain_db);
		fwrite(image.bp, sizeof(char), (int)image.width*(int)image.height*2, pFile);
		fclose(pFile);
		time(&t);
		p = localtime(&t);
		fwrite(p, sizeof(tm), 1 , pFile_time);
		fwrite(&exposure_time, sizeof(int), 1 , pFile_time);
		fwrite(&gain, sizeof(int), 1 , pFile_time);
		//printf("write end \n");
		//usleep(1000);
	}
	time(&t);
	p = localtime(&t);
	printf("Stopping acquisition...(%d, %d, %d)\n",p->tm_hour,p->tm_min,p->tm_sec);
	fclose(pFile_time);
	xiStopAcquisition(xiH);
	xiCloseDevice(xiH);
	/*
#define EXPECTED_IMAGES 100
	for (int images = 0; images < EXPECTED_IMAGES; images++)
	{
		// getting image from camera
		sprintf(filename, "%s/image_%d.bin", foldername, images);
		pFile = fopen(filename, "wb");
		stat = xiGetImage(xiH, 5000, &image);
		HandleResult(stat, "xiGetImage");
		unsigned char pixel = *(unsigned char*)image.bp;
		printf("Image %d (%dx%d) received from camera. First pixel value: %d,exposure: %d,gain: %f\n", images, (int)image.width, (int)image.height, pixel, image.exposure_time_us, (float)image.gain_db);
		fwrite(image.bp, sizeof(char), (int)image.width*(int)image.height*2, pFile);
		fclose(pFile);
	}
*/
	
finish:
	printf("Done\n");
#ifdef WIN32
	Sleep(2000);
#endif
	return 0;
}
