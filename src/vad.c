#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "vad.h"
#include "pav_analysis.h"

const float FRAME_TIME = 10.0F; /* in ms. */

/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
  "UNDEF", "S", "V", "INIT"
};

const char *state2str(VAD_STATE st) {
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct {
  float zcr;
  float p;
  float am;
} Features;

Features compute_features(const float *x, int N) {
  /*
   * Input: x[i] : i=0 .... N-1 
   * Ouput: computed features
   */

  Features feat;

  feat.p = compute_power(x,N);
  return feat;
}

VAD_DATA * vad_open(float rate) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  vad_data->init_count = 0;
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {
  VAD_STATE state = ST_SILENCE; /* All audios typically end in silence */

  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

VAD_STATE vad(VAD_DATA *vad_data, float *x, float alpha1, float alpha2, int *maybe_voice_run, int *maybe_silence_run, int to_init) {

  Features f = compute_features(x, vad_data->frame_length);
  vad_data->last_feature = f.p; /* save feature, in case you want to show */

  // Change of state based on: reference power value from initialization frames + basic hysterisis + needs several frames obeying same condition to change state
  switch (vad_data->state) {
  case ST_INIT:
    vad_data->p0 += f.p + alpha2; // Upper threshold to silence (starting power + alpha2)
    vad_data->p1 += f.p + alpha2 + alpha1; // Lower threshold to voice (starting power + alpha2 + alpha1)

    vad_data->init_count++;
    
    if (vad_data->init_count == to_init){ // After doing the necessary init frames, we get the mean thresholds and go to ST_SILENCE
      vad_data->p0 = vad_data->p0 / to_init;
      vad_data->p1 = vad_data->p1 / to_init;
      vad_data->state = ST_SILENCE;
    }
    break;

  case ST_SILENCE:
    if (f.p > vad_data->p1)
      vad_data->state = ST_UNDEF;
    break;

  case ST_VOICE:
    if (f.p < vad_data->p0)
      vad_data->state = ST_UNDEF;
    break;

  /* Return to previous state if we've stopped obeying the rule, if not stay in UNDEF. Returning to previous state won't write anything in the V/S report (False Alarm) */
  case ST_UNDEF: 
    if (f.p < vad_data->p0 && *maybe_voice_run>0){ /* If it looks like silence and we were thinking it was voice... Return! */
      vad_data->state = ST_SILENCE;
      *maybe_voice_run = 0;
    }else if (f.p > vad_data->p1 && *maybe_silence_run>0){ /* If it looks like voice and we were thinking it was silence... Return! */
      vad_data->state = ST_VOICE;
      *maybe_silence_run = 0;    
    }
    break;
  }

  return vad_data->state;
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}
