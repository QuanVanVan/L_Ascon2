#ifndef ROUND_H_
#define ROUND_H_

#include "ascon.h"
#include "constants.h"
#include "forceinline.h"
#include "printstate.h"
#include "word.h"
#include "l_ascon2_layer.h"

forceinline void ROUND(ascon_state_t* s, uint8_t C) {
  ascon_state_t t;
  /* round constant */
  s->x[2] ^= C;
  /* s-box layer */
  s->x[0] ^= s->x[4];
  s->x[4] ^= s->x[3];
  s->x[2] ^= s->x[1];
  t.x[0] = s->x[0] ^ (~s->x[1] & s->x[2]);
  t.x[2] = s->x[2] ^ (~s->x[3] & s->x[4]);
  t.x[4] = s->x[4] ^ (~s->x[0] & s->x[1]);
  t.x[1] = s->x[1] ^ (~s->x[2] & s->x[3]);
  t.x[3] = s->x[3] ^ (~s->x[4] & s->x[0]);
  t.x[1] ^= t.x[0];
  t.x[3] ^= t.x[2];
  t.x[0] ^= t.x[4];
  t.x[2] = ~t.x[2];
  /* linear layer: L_Ascon2 thay cho p_L */
  L_ASCON2_LAYER(t.x);
  s->x[0] = t.x[0];
  s->x[1] = t.x[1];
  s->x[2] = t.x[2];
  s->x[3] = t.x[3];
  s->x[4] = t.x[4];
  printstate(" round output", s);
}

forceinline void PROUNDS(ascon_state_t* s, int nr) {
  int i = START(nr);
  do {
    ROUND(s, RC(i));
    i += INC;
  } while (i != END);
}

#endif /* ROUND_H_ */
