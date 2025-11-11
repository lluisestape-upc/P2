#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sndfile.h>

#include "vad.h"
#include "vad_docopt.h"

#define DEBUG_VAD 0x1

int main(int argc, char *argv[]) {
  int verbose = 0; /* To show internal state of vad: verbose = DEBUG_VAD; */

  SNDFILE *sndfile_in, *sndfile_out = 0;
  SF_INFO sf_info;
  FILE *vadfile;
  int n_read = 0, i;

  VAD_DATA *vad_data;
  VAD_STATE state, last_state;

  float *buffer, *buffer_zeros;
  int frame_size;         /* in samples */
  float frame_duration;   /* in seconds */
  unsigned int t, last_t; /* in frames */
  float alpha1, alpha2;   /* threshold offsets from first power level */
  int to_voice_needed, to_silence_needed; /* frames needed (obeying same rules) to actually change state */
  int maybe_voice_run, maybe_silence_run; /* frame count for UNDEF state */
  int to_init; /* frames needed to initialize S power reference value */

  char	*input_wav, *output_vad, *output_wav;

  DocoptArgs args = docopt(argc, argv, /* help */ 1, /* version */ "2.0");

  verbose    = args.verbose ? DEBUG_VAD : 0;
  input_wav  = args.input_wav;
  output_vad = args.output_vad;
  output_wav = args.output_wav;
  alpha1     = atof(args.alpha1);
  alpha2     = atof(args.alpha2);
  to_voice_needed = atof(args.to_voice);
  to_silence_needed = atof(args.to_silence);
  maybe_voice_run = 0;
  maybe_silence_run = 0;
  to_init = atof(args.to_init);

  if (input_wav == 0 || output_vad == 0) {
    fprintf(stderr, "%s\n", args.usage_pattern);
    return -1;
  }

  /* Open input sound file */
  if ((sndfile_in = sf_open(input_wav, SFM_READ, &sf_info)) == 0) {
    fprintf(stderr, "Error opening input file %s (%s)\n", input_wav, strerror(errno));
    return -1;
  }

  if (sf_info.channels != 1) {
    fprintf(stderr, "Error: the input file has to be mono: %s\n", input_wav);
    return -2;
  }

  /* Open vad file */
  if ((vadfile = fopen(output_vad, "wt")) == 0) {
    fprintf(stderr, "Error opening output vad file %s (%s)\n", output_vad, strerror(errno));
    return -1;
  }

  /* Open output sound file, with same format, channels, etc. than input */
  if (output_wav) {
    if ((sndfile_out = sf_open(output_wav, SFM_WRITE, &sf_info)) == 0) {
      fprintf(stderr, "Error opening output wav file %s (%s)\n", output_wav, strerror(errno));
      return -1;
    }
  }

  vad_data = vad_open(sf_info.samplerate);
  /* Allocate memory for buffers */
  frame_size   = vad_frame_size(vad_data);
  buffer       = (float *) malloc(frame_size * sizeof(float));
  buffer_zeros = (float *) malloc(frame_size * sizeof(float));
  for (i=0; i< frame_size; ++i) buffer_zeros[i] = 0.0F;

  frame_duration = (float) frame_size/ (float) sf_info.samplerate;
  last_state = ST_UNDEF;

  for (t = last_t = 0; ; t++) { /* For each frame ... */
    /* End loop when file has finished (or there is an error) */
    if  ((n_read = sf_read_float(sndfile_in, buffer, frame_size)) != frame_size) break;

    if (sndfile_out != 0) {
      /* TODO: before doing anything, copy all the samples into sndfile_out */
    }

    state = vad(vad_data, buffer, alpha1, alpha2, &maybe_voice_run, &maybe_silence_run, to_init);
    if (verbose & DEBUG_VAD) vad_show_state(vad_data, stdout);

    if (state != last_state) {
      if (state == ST_UNDEF){ /* If we're in an UNDEF state...*/
        if(last_state == ST_SILENCE){ /* If we were in silence, now it can be voice */
          maybe_voice_run++;
          if(maybe_voice_run == to_voice_needed && t != last_t){ /* If we reached the necessary num of frames... */
            state = vad_data->state = ST_VOICE;
            maybe_voice_run = 0;

            fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, (t-(to_voice_needed-1)) * frame_duration, state2str(last_state)); /* Remember that the state actually changed "to_voice_needed" frames ago */
            last_state = state;
            last_t = t-(to_voice_needed-1);
          }
        }else{ /* If we were in voice, now it can be silence */
          maybe_silence_run++;
          if(maybe_silence_run == to_silence_needed && t != last_t){ /* If we reached the necessary num of frames... */
            state = vad_data->state = ST_SILENCE;
            maybe_silence_run = 0;

            fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, (t-(to_silence_needed -1)) * frame_duration, state2str(last_state)); /* Remember that the state actually changed "to_silence_needed" frames ago */
            last_state = state;
            last_t = t-(to_silence_needed -1);
          }
        }
      }else if (state == ST_SILENCE){ /* This only happens after initialization, when we start with last_state=ST_UNDEF and we go from state=ST_INIT to state=ST_SILENCE */
        last_state = state;
      }
    }

    if (sndfile_out != 0) {
      /* TODO: go back necessary frames and write zeros in silence segments if we change to ST_VOICE */
    }
  }

  state = vad_close(vad_data); /* Returns ST_SILENCE and frees vad_data */

  /* We typically end in silence */
  if (t != last_t){
    if (last_state!=ST_SILENCE) { /* If the last_state recorded wasn't ST_SILENCE, we were most surely in an uncompleted maybe_silence_run. Make those final UNDEF frames silence */
      fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, (t-(maybe_silence_run-1)) * frame_duration / (float) sf_info.samplerate, state2str(last_state));
    fprintf(vadfile, "%.5f\t%.5f\t%s\n", (t-(maybe_silence_run-1)) * frame_duration, t * frame_duration + n_read / (float) sf_info.samplerate, state2str(state));
    } else{ /* If the last state was ST_SILENCE, don't forget to report it! */
      fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, t * frame_duration + n_read / (float) sf_info.samplerate, state2str(last_state));
    }
  }

  /* clean up: free memory, close open files */
  free(buffer);
  free(buffer_zeros);
  sf_close(sndfile_in);
  fclose(vadfile);
  if (sndfile_out) sf_close(sndfile_out);
  return 0;
}
