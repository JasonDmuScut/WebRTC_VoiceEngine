/*
 *  Copyright (c) 2013 Gary Yu. All Rights Reserved.
 *
 *	URL: https://github.com/garyyu/WebRTC_VoiceEngine
 *
 *  Use of this source code is governed by a New BSD license which can be found in the LICENSE file
 *  in the root of the source tree. Refer to README.md.
 *  For WebRTC License & Patents information, please read files LICENSE.webrtc and PATENTS.webrtc.
 */

/*
 * This file contains the Usage Demo of this WebRTC AEC.
 *
 */

#include <iostream>
#include <math.h>
#include <assert.h>
#include <string.h>


#if	!defined(_WEBRTC_API_EXPORTS)

using namespace std;

FILE *rfile = NULL;
FILE *wfile = NULL;

void PrintOptions()
{
    cout << "WebRTC AEC Demo" <<endl;
    cout << "3. Local File Test"<<endl;
    cout << "0. End"<<endl;
    cout << "    Please select your test item:";
}

#define Float2Int16(flSample) (int16_t)floor(32768*flSample+0.5f)
#define IntToFloat(iSample)   (float)(iSample/32768.0f)

void LocalFileTest();

int main(int argc, char **argv)
{

    PrintOptions();
    int i = -1;
    cin >> i;
    while (i != 0)
    {
        switch(i)
        {
        case 3: LocalFileTest();break;
        default:break;
        }
        
        PrintOptions();
        cin >> i;
    }

    return 0;
}

void LocalFileTest()
{   
	unsigned i = 0;
	size_t	 read_size = 0;
	unsigned total_size= 0;

	webrtc_ec * echo = NULL;
	unsigned	clock_rate = 16000;			//16kHz
	unsigned	samples_per_frame = 16*10;	//16kHz*10ms
	unsigned	system_delay = 0;

	int16_t	  * file_buffer = NULL;
	int16_t   * farend_buffer  = NULL;
	int16_t   * nearend_buffer = NULL;

    char strFileIn [128] = "stero_in.pcm";	//Left  Channel: Farend Signal, Right Channel: Nearend Input Signal (with echo)
    char strFileOut[128] = "stero_out.pcm";	//Right Channel: same as input, Right Channel: Nearend Output after AEC.

    cout << "    Please give input file name:";
	cin >> strFileIn;

#ifdef WIN32
    fopen_s(&rfile, strFileIn, "rb");
#else
  rfile = fopen(strFileIn, "rb");
#endif
  
	if (rfile == NULL){
		printf("file %s open fail.\n", strFileIn);
		return;
	}
    fopen_s(&wfile, strFileOut,"wb");
	assert(wfile!=NULL);

    cout << "    Sound Card Clock Rate (kHz)?:";
	cin >> clock_rate;
	if ((clock_rate!=8) && (clock_rate!=16) && (clock_rate!=32) && (clock_rate!=48))
	{
		printf("Not Supported for your %d kHz.\n", clock_rate);
		fclose(rfile); rfile=NULL;
		fclose(wfile); wfile=NULL;
		return;
	}
	samples_per_frame = clock_rate*10;	//10ms sample numbers
	clock_rate *= 1000;	// kHz -> Hz

    cout << "    System Delay (sound card buffer & application playout buffer) (ms)?:";
	cin >> system_delay;
	if (system_delay>320){	//To Be check
		printf("Not Supported for your system delay %d (ms).\n", system_delay);	
		fclose(rfile); rfile=NULL;
		fclose(wfile); wfile=NULL;
		return;
	}

	if (0 != webrtc_aec_create(
                        clock_rate,
                        1,		// channel_count
                        samples_per_frame,	// clock_rate(kHz)*10(ms)
                        system_delay,		// system_delay (ms)
                        0,		// options,
                        (void**)&echo ) )
	{
		printf("%s:%d-Error on webrtc_aec_create()!\n", __FILE__, __LINE__);
		fclose(rfile);
		fclose(wfile);
		return;
	}

	file_buffer = (int16_t *)malloc( samples_per_frame * 2 * 2 );	//2 Bytes/Sample, 2 Channels: Left for Farend, Right for Nearend.
	assert( file_buffer != NULL );
	farend_buffer  = (int16_t *)malloc( samples_per_frame * 2 );	//2 Bytes/Sample
	nearend_buffer = (int16_t *)malloc( samples_per_frame * 2 );	//2 Bytes/Sample
	assert( farend_buffer != NULL );
	assert( nearend_buffer!= NULL );

	while(1)
	{
		read_size = fread(file_buffer, sizeof(int16_t), 2*samples_per_frame, rfile);
		total_size += read_size;
		if (read_size != (2*samples_per_frame)){
			printf("File End. Total %d Bytes.\n", total_size<<1);
			break;
		}

		// Split into Farend and Nearend Signals
		for (i=0; i<samples_per_frame; i++){
			farend_buffer [i] = file_buffer[(i<<1)+0];
			nearend_buffer[i] = file_buffer[(i<<1)+1];
		}

		// Echo Processing
		if (0 != webrtc_aec_cancel_echo( echo,
						   nearend_buffer,	// rec_frm
						   farend_buffer,	// play_frm
						   samples_per_frame,
						   0,	// options
						   NULL	// reserved 
						   ) )
		{
			printf("%s:%d-Error on webrtc_aec_cancel_echo()!\n", __FILE__, __LINE__);
			break;
		}

		// Merge Again to Stero, Left: Farend, Right: Nearend.
		for (i=0; i<samples_per_frame; i++){
			file_buffer[(i<<1)+1] = nearend_buffer[i];
		}

		// Save to Output File
		fwrite(file_buffer, sizeof(int16_t), 2*samples_per_frame, wfile);
	}

	//Get the AEC Status
	MyAecMetrics aecMetrics;
	if (0 == webrtc_aec_get_metrics(echo, &aecMetrics))
	{
		printf("ERL (Ave/Max/Min)=%d/%d/%d \n",aecMetrics.erl.average,aecMetrics.erl.max,aecMetrics.erl.min);
		printf("ERLE(Ave/Max/Min)=%d/%d/%d \n",aecMetrics.erle.average,aecMetrics.erle.max,aecMetrics.erle.min); 
		printf("aNLP(Ave/Max/Min)=%d/%d/%d \n",aecMetrics.aNlp.average,aecMetrics.aNlp.max,aecMetrics.aNlp.min); 
	}

	int median_delay, standard_delay;
	if (0 == webrtc_aec_get_delay_metrics(echo, &median_delay, &standard_delay))
	{
		printf("WebRTC AEC Median Delay Estimation: %d(ms), Std Delay Estimation: %d(ms)\n", median_delay, standard_delay);
	}

	webrtc_aec_destroy( echo );

	fclose(rfile); rfile=NULL;
	fclose(wfile); wfile=NULL;

	free(file_buffer);
	free(farend_buffer);
	free(nearend_buffer);
	return;
}

#endif	//_WEBRTC_API_EXPORTS