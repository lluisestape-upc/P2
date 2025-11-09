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

/* 
 * TODO: Delete and use your own features!
 */

Features compute_features(const float *x, int N) {
  /*
   * Input: x[i] : i=0 .... N-1 
   * Ouput: computed features
   */
  /* 
   * DELETE and include a call to your own functions
   *
   * For the moment, compute random value between 0 and 1 
   */
  Features feat;
  // feat.zcr = feat.p = feat.am = (float) rand()/RAND_MAX;

  feat.p = compute_power(x,N);
  return feat;
}

/* 
 * TODO: Init the values of vad_data
 */

VAD_DATA * vad_open(float rate) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {
  /* 
   * TODO: decide what to do with the last undecided frames
   */
  VAD_STATE state = vad_data->state;

  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

/* 
 * TODO: Implement the Voice Activity Detection 
 * using a Finite State Automata
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x, float alpha1, float alpha2, int *maybe_voice_run, int *maybe_silence_run) {

  /* 
   * TODO: You can change this, using your own features,
   * program finite state automaton, define conditions, etc.
   */

  Features f = compute_features(x, vad_data->frame_length);
  vad_data->last_feature = f.p; /* save feature, in case you want to show */

  // Thresholds based on basic hysterisis, needs several frames obeying condition to change state (to_voice_needed, to_silence_needed)
  switch (vad_data->state) {
  case ST_INIT:
    vad_data->p0 = f.p + alpha2; // Upper threshold to silence (starting power + alpha2)
    vad_data->p1 = vad_data->p0 + alpha1; // Lower threshold to voice (starting power + alpha2 + alpha1)
    vad_data->state = ST_SILENCE;
    break;

  case ST_SILENCE:
    if (f.p > vad_data->p1)
      vad_data->state = ST_UNDEF;
    break;

  case ST_VOICE:
    if (f.p < vad_data->p0)
      vad_data->state = ST_UNDEF;
    break;

  // case ST_UNDEF: /* Return to previous state if we've stopped obeying the rule, if not stay in UNDEF. Returning to previous state won't write anything in the V/S report */
  //   if (f.p < vad_data->p1 && *maybe_voice_run>0){ /* If we stopped obeying the MV rule and we were thinking it was voice... */
  //     vad_data->state = ST_SILENCE;
  //     *maybe_voice_run = 0;
  //   }else if (f.p > vad_data->p0 && *maybe_silence_run>0){ /* If we stopped obeying the MS rule and we were thinking it was silence... */
  //     vad_data->state = ST_VOICE;
  //     *maybe_silence_run = 0;    
  //   }
  //   break;

    case ST_UNDEF: /* Return to previous state if we've stopped obeying the rule, if not stay in UNDEF. Returning to previous state won't write anything in the V/S report */
    if (f.p < vad_data->p0 && *maybe_voice_run>0){ /* If we now think it's silence and we were thinking it was voice... */
      vad_data->state = ST_SILENCE;
      *maybe_voice_run = 0;
    }else if (f.p > vad_data->p1 && *maybe_silence_run>0){ /* If we now think it's voice and we were thinking it was silence... */
      vad_data->state = ST_VOICE;
      *maybe_silence_run = 0;    
    }
    break;
  }

  if (vad_data->state == ST_SILENCE ||
      vad_data->state == ST_VOICE)
    return vad_data->state;
  else
    return ST_UNDEF;
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}
