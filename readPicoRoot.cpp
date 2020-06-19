#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

#include <libusbtc08/usbtc08.h>





#include <string>
#include <iostream>
#include <csignal>
#include <getopt.h>

#include <sys/time.h> /* struct timeval, select() */

#include "TFile.h"
#include "TTree.h"

using namespace std;
ULong64_t sys_time0 = 0;
ULong64_t sys_time_temp = 0;
ULong64_t sys_time = 0;
ULong64_t sys_time_last = 0;
ULong64_t sec_time_2020;
UInt_t day_loops;

time_t rawtime;
struct tm *timinfo;


#define Sleep(a) usleep(1000*a)
#define scanf_s scanf
#define fscanf_s fscanf
#define memcpy_s(a,b,c,d) memcpy(a,c,d)

typedef enum enBOOL
{
	FALSE, TRUE
} BOOL;


ULong64_t getTime()
{


  time(&rawtime);
  timinfo = gmtime(&rawtime);
  sec_time_2020 = ((timinfo->tm_year-120)*365*86400) + (timinfo->tm_yday*86400) + (timinfo->tm_hour*3600)  + (timinfo->tm_min*60) + timinfo->tm_sec;
    
  struct timeval t1;
  struct timezone tz;
  gettimeofday(&t1, &tz);
  sys_time_temp = (t1.tv_sec) * 1000000 + t1.tv_usec;

  if(sys_time0 == 0)
    sys_time0 = sys_time_temp;

  sys_time = sys_time_temp - sys_time0 + (day_loops*24*60*60*1000000);

  //We're in a new day;
  if(sys_time < 0)
    {
      day_loops++;
      sys_time = sys_time_temp - sys_time0 + (day_loops*24*60*60*1000000);
    }
  
  
  return sys_time;
}





#define PREF4 __stdcall

#define BUFFER_SIZE 1000	// Buffer size to be used for streaming mode captures






using namespace std;


bool external_kill = false;

void signalHandler( int signum ) {
   cout << "Interrupt signal (" << signum << ") received.\n";
   cout << "Going to shut things down (" << signum << ") properly (I hope).\n";
   external_kill = true;
   
}

void usage()
{
  cout << "USAGE:" << std::endl
       << "./readPicoRoot --duration_sec <nseconds> [options]" << std::endl << std::endl
       << "OPTIONS:" << std::endl
       << "-d <nseconds> , --duration_sec <nseconds>" << std::endl
       << "\t Set the duration of the acquisition"<< std::endl << std::endl
       << "-h, --help" << std::endl
       << "\t Show this help menu" << std::endl << std::endl
       << "-o <output dir> , --output_dir <output dir>" << std::endl
       << "\t Set the path used for the output rootfile etc" << std::endl << std::endl
       << "-r <run number> , --run_number <run number>" << std::endl
       << "\t Set the run number used for filenameing etc" << std::endl << std::endl 
       << "-u <time in seconds> , --update_interval <time in seconds>" << std::endl
       << "\t Set the time interval between re-plotting of the hists and or traces" << std::endl << std::endl;
      
}


int16_t handle = 0;								  /* The handle to a TC-08 returned by usb_tc08_open_unit() or usb_tc08_open_unit_progress() */
int8_t selection = 0;								/* User selection from the main menu */

float temp[USBTC08_MAX_CHANNELS + 1] = {0.0};		/* Buffer to store single temperature readings from the TC-08 */
float * temp_buffer[USBTC08_MAX_CHANNELS + 1];		/* 2D array to be used for streaming data capture */
int32_t times_buffer[BUFFER_SIZE] = {0};			/* Array to hold the time of the conversion of the first channel for each set of readings in streaming mode captures */
int16_t overflows[USBTC08_MAX_CHANNELS + 1] = {0};	/* Array to hold overflow flags for each channel in streaming mode captures */

int32_t	channel = 0; 								/* Loop counter for channels */
uint32_t reading = 0;								/* Loop counter for readings */
int32_t retVal = 0;									/* Return value from driver calls indication success / error */
USBTC08_INFO unitInfo;								/* Struct to hold unit information */

uint32_t numberOfReadings = 0;							/* Number of readings to collect in streaming mode */
int32_t readingsCollected = 0;							/* Number of readings collected at a time in streaming mode */
uint32_t totalReadings[USBTC08_MAX_CHANNELS + 1] = {0};	/* Total readings collected in streaming mode */

TTree *picotemp = 0;

bool tryread;
bool busyreading;

void read_temp()
{

  if(busyreading == false)
    {
      busyreading = true;
      usb_tc08_get_single(handle, temp, NULL, USBTC08_UNITS_CENTIGRADE);
      
      printf(" done!\n\nCJC      : %3.2f C\n", temp[0]);
      
      for (channel = (int32_t) USBTC08_CHANNEL_1; channel < USBTC08_MAX_CHANNELS + 1; channel++)
	{
	  printf("Channel %d: %3.2f C\n", channel, temp[channel]);
	}
      
      picotemp->Fill();
      busyreading = false;
    }
}


int main(int argc, char** argv)
{

  bool firstread = true;
  tryread = false;
  busyreading = false;


  
  //gErrorIgnoreLevel = kError;
  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);
  
  ULong64_t acquisition_runtime = 0;
  string f_path = "./";
  bool showhelp = false;
  bool bdefault = false;
  bool bunknown = false;
  ULong64_t plotinterval = 10;
  Int_t runnumber = 111;
  
  
  int c;
  while(true)
    {
      int option_index = 0;
      static struct option long_options[] =
	{
	  {"duration_sec",required_argument,NULL, 'd'},
	  {"output_dir",required_argument,NULL, 'o'},
	  {"run_number",required_argument,NULL, 'r'},
	  {"update_interval ",required_argument,NULL, 'u'},
	  {"help",no_argument,NULL, 'h'},
	  {0, 0, 0, 0}
	};

      c = getopt_long_only(argc, argv, "d:o:r:u:h",long_options, &option_index);
      if (c == -1)
	break;

      switch (c)
        {

	case 'd':
	  acquisition_runtime = atoi(optarg)*1000000;;
	  break;

	case 'h':
	  showhelp = true;
	  break;
	  
	case 'o':
	  f_path = string(optarg);
	  break;

	case 'r':
	  runnumber = atoi(optarg);
	  break;

	case 'u':
	  plotinterval = atoi(optarg)*1000000;
	  break;
	  
        case '?':
	  bunknown = true;
	  break;

        default:
	  bdefault = true;
        }
    }


  if(showhelp || bunknown || bdefault)
    {
      usage();
      exit(0);
    }


  if(acquisition_runtime < 1)
    {
      printf("Please indicate a run duration with the -d option\n");fflush(stdout);
      usage();
      exit(0);
    }

  
  
 
  
  

  char fname[1000];
  sprintf(fname,"%s/pico.root",f_path.c_str());

  TFile *fraw = new TFile(fname,"RECREATE");
  picotemp = new TTree("picotemp","picotemp");
  picotemp->Branch("sec_time_2020",&sec_time_2020,"sec_time_2020/l");
  picotemp->Branch("temp",temp,"temp[9]/F");
  picotemp->Branch("runnumber",&runnumber,"runnumber/I");

  int32_t minimumIntervalMs = 0; /* Minimum time interval between samples. */

  /* Print header information */
  printf ("Pico Technology USB TC-08 Console Example Program\n");
  printf ("-------------------------------------------------\n\n");
  printf ("Looking for USB TC-08 devices on the system.\n\n");
  printf ("Progress: ");
	
	
 	
  /* Try to open one USB TC-08 unit, if available 
   * The simplest way to open the unit like is this:
   *
   *   handle = usb_tc08_open_unit();
   *
   * but that will cause your program to wait while the driver downloads
   * firmware to any connected TC-08 units. If you're making an 
   * interactive application, it's better to use 
   * usb_tc08_open_unit_async() which returns immediately and allows you to 
   * display some sort of progress indication to the user as shown below: 
   */
  retVal = usb_tc08_open_unit_async();
	
  /* Make sure no errors occurred opening the unit */
  if (!retVal) 
    {
      printf ("\n\nError opening unit. Exiting.\n");
      return -1;
    }

  /* Display a text "progress bar" while waiting for the unit to open */
  while ((retVal = usb_tc08_open_unit_progress(&handle, NULL)) == USBTC08_PROGRESS_PENDING)
    {
      /* Update our "progress bar" */
      printf("|");
      fflush(stdout);
      Sleep(200);
    }
		
  /* Determine whether a unit has been opened */
  if (retVal != USBTC08_PROGRESS_COMPLETE || handle <= 0) 
    {
      printf ("\n\nNo USB TC-08 units could be opened. Exiting.\n");
      return -1;
    } 
  else 
    {
      printf ("\n\nUSB TC-08 opened successfully.\n");
    }
	
  /* Get the unit information */
  unitInfo.size = sizeof(unitInfo);
  usb_tc08_get_unit_info(handle, &unitInfo);
	
  printf("\nUnit information:\n");
  printf("Driver: %s \nSerial: %s \nCal date: %s \n", unitInfo.DriverVersion, unitInfo.szSerial, unitInfo.szCalDate);

  /* Set up all channels */
  retVal = usb_tc08_set_channel(handle, 0,'C');

  for (channel = 1; channel < (USBTC08_MAX_CHANNELS + 1); channel++)
    {
      retVal &= usb_tc08_set_channel(handle, channel,'K');
    }
	
  /* Make sure this was successful */
  if (retVal)
    {
      printf("\nEnabled all channels, selected Type K thermocouple.\n");
    } 
  else 
    {
      printf ("\n\nError setting up channels. Exiting.\n");
      usb_tc08_close_unit(handle);
      Sleep(2000);
      return -1;
    }


  
  getTime();
  Double_t sys_time_last = sys_time;
  
  while(!external_kill && sys_time < acquisition_runtime)
    {

      if(sys_time - sys_time_last > plotinterval || firstread)
	{
	  tryread = true;
	}
      else
	{
	  tryread = false;
	}
      

      if(tryread == true && busyreading == false)
	{

	  if(firstread)
	    firstread = false;

	  cout << Double_t(sys_time)/1e6 << endl;
	  sys_time_last =  sys_time;
	  read_temp();
	}
      
      Sleep(300);
      getTime();
    }
  
  fraw->Write();
  
  usb_tc08_close_unit(handle);
  return 0;
}
