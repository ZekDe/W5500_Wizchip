#ifndef EDGE_DETECTION_H
#define EDGE_DETECTION_H

#include "stdint.h"

typedef struct
{
  uint8_t aux;
}edge_detection_t;

uint8_t edgeDetection(edge_detection_t *obj, uint8_t catch);

#endif
