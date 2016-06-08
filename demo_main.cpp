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
#include <sys/select.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/audio_processing/aec/include/echo_cancellation.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/include/trace.h"
#include "webrtc/common_audio/include/audio_util.h"
#include "webrtc/typedefs.h"

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
  
//  PrintOptions();
//  int i = -1;
//  cin >> i;
//  while (i != 0)
//  {
//    switch(i)
//    {
//      case 3: LocalFileTest();break;
//      default:break;
//    }
//    
//    PrintOptions();
//    cin >> i;
//  }
  cout << "Local file test\n";
  LocalFileTest();
  
  return 0;
}

void LocalFileTest()
{
  unsigned i = 0;
  size_t	 read_size = 0;
  unsigned total_size= 0;
  
  //  webrtc_ec * echo = NULL;
  unsigned	clock_rate = 48;			//16 (kHz)
  unsigned	samples_per_frame = clock_rate*10;	//16kHz*10ms
  unsigned	system_delay = 0;
  
  int16_t	  * file_buffer    = NULL;
  float     * farend_buffer  = NULL;
  float     * nearend_buffer = NULL;
  
  int analog_level;
  bool has_voice;
  
  char strFileIn [128] = "wrtctest_48khz_in.raw";	//Left  Channel: Farend Signal, Right Channel: Nearend Input Signal (with echo)
  char strFileOut[128] = "wrtctest_48khz_out.raw";	//Left Channel: same as input, Right Channel: Nearend Output after AEC.
  
//  cout << "    Please give input file name:";
//  cin >> strFileIn;
  
#ifdef WIN32
  fopen_s(&rfile, strFileIn, "rb");
#else
  rfile = fopen(strFileIn, "rb");
#endif
  
  if (rfile == NULL){
    printf("file %s open fail.\n", strFileIn);
    return;
  }
#ifdef WIN32
  fopen_s(&wfile, strFileOut,"wb");
#else
  wfile = fopen(strFileOut, "wb");
#endif
  assert(wfile!=NULL);
  
//  cout << "    Sound Card Clock Rate (kHz)?:";
//  cin >> clock_rate;
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
  system_delay = 50;
//  cin >> system_delay;
  if (system_delay>320){	//To Be check
    printf("Not Supported for your system delay %d (ms).\n", system_delay);
    fclose(rfile); rfile=NULL;
    fclose(wfile); wfile=NULL;
    return;
  }
  // audio_processing.h example updated
  // APM accepts only linear PCM audio data in chunks of 10 ms. The int16
  // interfaces use interleaved data, while the float interfaces use deinterleaved
  // data.
  //
  // Usage example, omitting error checking:
  webrtc::AudioProcessing* apm = webrtc::AudioProcessing::Create();
  
  apm->high_pass_filter()->Enable(true);
  
  apm->echo_cancellation()->enable_drift_compensation(false);
  apm->echo_cancellation()->enable_metrics(true);
//  apm->echo_cancellation()->enable_comfort_noise(true);
  apm->echo_cancellation()->Enable(true);
  
  
  apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kHigh);
  apm->noise_suppression()->Enable(true);
  
  apm->gain_control()->set_analog_level_limits(0, 255);
  apm->gain_control()->set_mode(webrtc::GainControl::kAdaptiveAnalog);
  apm->gain_control()->Enable(false);
  
  apm->voice_detection()->Enable(true);
  
  file_buffer = (int16_t *)malloc( samples_per_frame * 2 * 2 );	//2 Bytes/Sample, 2 Channels: Left for Farend, Right for Nearend.
  assert( file_buffer != NULL );
  farend_buffer  = (float *)malloc( samples_per_frame * sizeof(float) );	//4 Bytes/Sample
  nearend_buffer = (float *)malloc( samples_per_frame * sizeof(float) );	//4 Bytes/Sample
  assert( farend_buffer != NULL );
  assert( nearend_buffer!= NULL );
  
  //
  // // Start a voice call...
  //
  webrtc::StreamConfig config(48000, 1, false);
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
      farend_buffer [i] = webrtc::S16ToFloat(file_buffer[(i<<1)+0]);
      nearend_buffer[i] = webrtc::S16ToFloat(file_buffer[(i<<1)+1]);
    }
    // // ... Render frame arrives bound for the audio HAL ...
    //    apm->ProcessReverseStream(AudioFrame *render_frame);
    apm->ProcessReverseStream(&farend_buffer, config, config, &farend_buffer);
    
    // // ... Capture frame arrives from the audio HAL ...
    // // Call required set_stream_ functions.
    apm->set_stream_delay_ms(-80); // -80 works well...
    apm->gain_control()->set_stream_analog_level(analog_level);
    //
    // apm->ProcessStream(AudioFrame *capture_frame);
    apm->ProcessStream(&nearend_buffer, config, config, &nearend_buffer);
    //
    // // Call required stream_ functions.
    analog_level = apm->gain_control()->stream_analog_level();
    has_voice = apm->voice_detection()->stream_has_voice();
//    system_delay = apm->stream_delay_ms();
//    printf("stream_delay %d\n", system_delay); // Never changes
    //
    // // Repeate render and capture processing for the duration of the call...
    // Echo Processing
    
    // Merge Again to Stereo, Left: Farend, Right: Nearend.
    for (i=0; i<samples_per_frame; i++){
      file_buffer[(i<<1)+1] = webrtc::FloatToS16(nearend_buffer[i]);
    }
    
    // Save to Output File
    fwrite(file_buffer, sizeof(int16_t), 2*samples_per_frame, wfile);
  }
  
  //Get the AEC Status
  AecMetrics aecMetrics;
  if (0 == WebRtcAec_GetMetrics(apm, (AecMetrics*)&aecMetrics) || 1 )
  {
    printf("ERL (Ave/Max/Min)=%d/%d/%d \n",aecMetrics.erl.average,aecMetrics.erl.max,aecMetrics.erl.min);
    printf("ERLE(Ave/Max/Min)=%d/%d/%d \n",aecMetrics.erle.average,aecMetrics.erle.max,aecMetrics.erle.min);
    printf("aNLP(Ave/Max/Min)=%d/%d/%d \n",aecMetrics.aNlp.average,aecMetrics.aNlp.max,aecMetrics.aNlp.min);
  }
  
  int median_delay, standard_delay;
  float  fraction_poor_delays;
  if (0 == WebRtcAec_GetDelayMetrics(apm, &median_delay, &standard_delay, &fraction_poor_delays) || 1)
  {
    printf("WebRTC AEC Median Delay Estimation: %d(ms), Std Delay Estimation: %d(ms), Fraction of poor delays: %f\n", median_delay, standard_delay, fraction_poor_delays);
  }
  
  // // Close the application...
  delete apm;
  fflush(wfile);
  
  fclose(rfile); rfile=NULL;
  fclose(wfile); wfile=NULL;
  
  free(file_buffer);
  free(farend_buffer);
  free(nearend_buffer);
  return;
}

#endif	//_WEBRTC_API_EXPORTS