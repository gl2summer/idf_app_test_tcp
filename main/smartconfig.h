#ifndef _SMART_CONFIG_H__
#define _SMART_CONFIG_H__


typedef enum{
	SC_EVT_NONE=0,
	SC_EVT_LINK,
	SC_EVT_LINK_OVER,
}SC_EVENT;


typedef bool (*smartconfig_cb_t)(SC_EVENT sc_evt, void *p_data);


void smartconfig_stop(void);
void smartconfig_restart(void);
void smartconfig_start(smartconfig_cb_t cb);


#endif //_SMART_CONFIG_H__

