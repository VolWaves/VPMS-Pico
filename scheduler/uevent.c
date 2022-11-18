
#include "uevent.h"

#ifndef ASSERT_ERR
	#define ASSERT_ERR(x)
#endif // !ASSERT_ERR

#define EVT_HANDLER_LENGTH (32)

__attribute__((weak)) void user_event_dispatcher(uevt_t evt) {
	LOG_RAW("[ERROR]event dispatcher NOT set!!!\r\n");
}

__attribute__((weak)) void user_event_handler(uevt_t* evt) {
#if G_LOG_ENABLED == 1 && EVT_LOG_ENABLED == 1
	LOG_RAW("EVT Pop:%04X\r\n", evt->evt_id);
#endif
	user_event_array_dispatcher(*evt);
}

void user_event_send(uevt_t evt, fpevt_h event_handler) {
	app_sched_event_put(&evt, event_handler);
}
void user_event_broadcast(uevt_t evt) {
	app_sched_event_put(&evt, user_event_handler);
}

fpevt_h evt_handler_array[EVT_HANDLER_LENGTH] = { NULL };
void user_event_init(void) {
	memset(evt_handler_array, 0, sizeof(evt_handler_array));
}

void user_event_handler_regist(fpevt_h func) {
	// 查询是否有重复注册
	for(uint8_t i = 0; i < EVT_HANDLER_LENGTH; i++) {
		if(evt_handler_array[i] == func) {
			return;
		}
	}
	// 插入空闲插槽
	for(uint8_t i = 0; i < EVT_HANDLER_LENGTH; i++) {
		if(evt_handler_array[i] == NULL) {
			// LOG_RAW("REG %x to %d\n",func,i);
			evt_handler_array[i] = func;
			return;
		}
	}
	// 队列满
	ASSERT_ERR(0);
}

void user_event_handler_unregist(fpevt_h func) {
	for(uint8_t i = 0; i < EVT_HANDLER_LENGTH; i++) {
		if(evt_handler_array[i] == func) {
			evt_handler_array[i] = NULL;
			return;
		}
	}
}

void user_event_array_dispatcher(uevt_t evt) {
	for(uint8_t i = 0; i < EVT_HANDLER_LENGTH; i++) {
		if(evt_handler_array[i] != NULL) {
			// LOG_RAW("dispatch %04x to array[%d]=%x\n",evt.evt_id,i,evt_handler_array[i]);
			(*(evt_handler_array[i]))(&evt);
		}
	}
}
