#include "edge_detection.h"

uint8_t edge_detection(edge_detection_t *obj, uint8_t catch)
{
   uint8_t retval = 0;
	
	if (catch)
	{
		if (!obj->aux)
		{
			obj->aux = 1;
			retval = 1;
		}
	}
	else
	{
		obj->aux = 0;
	}
	return retval;
}
