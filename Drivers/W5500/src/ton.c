#include "ton.h"

#define TIME_OVER(target,time) ((uint32_t)((time) - (target)) < 0x80000000U)

/**
 * \fn bool TON(uint8_t id, uint8_t in, uint32_t now, uint32_t preset_time)
 * \brief Start a timer with a specified duration as on-delay.
 * \param id active TON obj selection
 * \param in - timer is executed when the "in" state changes from 0 to 1
 * \param now - system tick continuously running
 * \param preset_time - timer is started for the time stored in
 * \return if time is over , return value is 1
 */
uint8_t TON(ton_t *obj, uint8_t in, uint32_t now, uint32_t preset_time)
{
	uint8_t ret_val = 0;

	if(in)
	{
		if(!obj->aux)
 		{
			obj->since = now + preset_time;
			obj->aux = 1;
 		}
		else if(TIME_OVER(obj->since, now))
		{
			ret_val = 1;
		}
	}
	else
	{
		obj->aux = 0;
	}

	return ret_val;
}
