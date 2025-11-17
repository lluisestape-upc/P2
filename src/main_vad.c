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
  unsigned int silence_segments_before_voice = 0; /* to count samples that may need to be zeroed later */

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
      sf_write_float(sndfile_out, buffer, n_read);
    }

    state = vad(vad_data, buffer, alpha1, alpha2, &maybe_voice_run, &maybe_silence_run, to_init);
    if (verbose & DEBUG_VAD) vad_show_state(vad_data, stdout);

    if (state != last_state) {
      if (state == ST_UNDEF){ /* If we're in an UNDEF state...*/
        if(last_state == ST_SILENCE){ /* If we were in silence, now it can be voice */
          maybe_voice_run++;
          silence_segments_before_voice++; /* Count samples that may need to be zeroed later */
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
      // If the state just changed to VOICE after UNDEF (that followed SILENCE)
      if ((last_t = t-(to_voice_needed-1)) && last_state == ST_VOICE) { // We just marked the end of the silence segment
          int samples_to_zero = silence_segments_before_voice;
          printf("Writing zeros at frame t=%u\n", t);
          // Seek backwards in the output file to the frames that were misclassified as silence
          sf_seek(sndfile_out, -samples_to_zero, SEEK_CUR);
          // Overwrite those frames with zeros (silence)
          sf_write_float(sndfile_out, buffer_zeros, samples_to_zero);
          // Set file pointer back to the end, ready for next frame
          sf_seek(sndfile_out, 0, SEEK_END);
          
          silence_segments_before_voice = 0;
      }
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

  //Zero out silence segments in the output WAV file if requested

    if (output_wav && output_vad) {
      SNDFILE *sndfile_rw = sf_open(output_wav, SFM_RDWR, &sf_info);
      if (sndfile_rw == NULL) {
          fprintf(stderr, "Error abriendo WAV output para modificar ceros: %s\n", output_wav);
      } else {
          FILE *vadfile_r = fopen(output_vad, "rt");
          if (vadfile_r == NULL) {
              fprintf(stderr, "Error abriendo archivo VAD para lectura: %s\n", output_vad);
              sf_close(sndfile_rw);
          } else {
              char line[256];
              float *zero_buffer = (float *)calloc(frame_size, sizeof(float));
              if (!zero_buffer) {
                  fprintf(stderr, "Error asignando memoria para buffer de ceros\n");
                  fclose(vadfile_r);
                  sf_close(sndfile_rw);
              } else {
                  while (fgets(line, sizeof(line), vadfile_r)) {
                      float start_time, end_time;
                      char state[16];
                      if (sscanf(line, "%f %f %15s", &start_time, &end_time, state) != 3)
                          continue;

                      if (state[0] == 'S' || state[0] == 's') {
                          sf_count_t sample_start = (sf_count_t)(start_time * sf_info.samplerate);
                          sf_count_t sample_end = (sf_count_t)(end_time * sf_info.samplerate);
                          if (sample_start < 0) sample_start = 0;
                          if (sample_end > sf_info.frames) sample_end = sf_info.frames;
                          sf_count_t total_samples = sample_end - sample_start;
                          sf_count_t written = 0;

                          // Seek to start sample
                          sf_seek(sndfile_rw, sample_start, SEEK_SET);

                          // Write zeros in chunks of frame_size
                          while (written < total_samples) {
                              sf_count_t block = (total_samples - written) > frame_size ? frame_size : (total_samples - written);
                              sf_write_float(sndfile_rw, zero_buffer, block);
                              written += block;
                          }
                      }
                  }
                  free(zero_buffer);
                  fclose(vadfile_r);
                  sf_close(sndfile_rw);
              }
          }
      }
    }

  //
  
  /* clean up: free memory, close open files */
  free(buffer);
  free(buffer_zeros);
  sf_close(sndfile_in);
  fclose(vadfile);
  if (sndfile_out) sf_close(sndfile_out);
  return 0;
}