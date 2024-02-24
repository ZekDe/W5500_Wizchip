#include "soft_timer.h"

#define TIME_PASSED(now, since)	(now >= since) ? (now - since) : (now + (UINT32_MAX - since))
#define TIME_OVER(target,time) ((uint32_t)((time) - (target)) < 0x80000000U)


/**
 * \fn void timer_check(soft_timer_t *obj, uint32_t now)
 * \brief Start a periodic timer with a specified duration .
 *
 * \param t - variables are stored which function needs
 * \param now - system tick continuously running
 * 
 */
void timerCheck(soft_timer_t *obj, uint32_t now)
{
	if(obj->in)
	{
		if(!obj->aux)
 		{
 			obj->since = now + obj->interval;
 			obj->aux = 1;
 		}
		else if(TIME_OVER(obj->since, now))
		{
				obj->since = now + obj->interval;
				obj->fp();
		}
		else
		{}
	}
	else
	{
		obj->aux = 0;
	}
}

/**
 * \fn void timerStart(timeout_t*, uint32_t)
 * \brief
 * \param s
 * \param interval
 */
void timerStart(soft_timer_t *obj)
{
	obj->in = 1;
}

/**
 * \fn void timerStop(timeout_t*)
 * \brief
 * \param s
 */
void timerStop(soft_timer_t *obj)
{
	obj->in = 0;
}

/**
* @brief void timer_set(soft_timer_t *obj, uint32_t interval, void(*fp)(void))
* @param id
* @param uint32_t interval
* @param void(*fp)(void)
* 
* @retval none
**/
void timerSet(soft_timer_t *obj, uint32_t interval, void(*fp)(void))
{
	obj->interval = interval;
	obj->fp = fp;
}
