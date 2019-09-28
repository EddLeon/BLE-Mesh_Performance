/* microbit.c - BBC micro:bit specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gpio.h>
#include <board.h>
#include <soc.h>
#include <misc/printk.h>
#include <ctype.h>
#include <gpio.h>
#include <pwm.h>

#include <display/mb_display.h>

#include <bluetooth/mesh.h>

#include "board.h"

#define SCROLL_SPEED   K_MSEC(300)

#define BUZZER_PIN     EXT_P0_GPIO_PIN
#define BEEP_DURATION  K_MSEC(60)

#define SEQ_PER_BIT  976
#define SEQ_PAGE     (NRF_FICR->CODEPAGESIZE * (NRF_FICR->CODESIZE - 1))
#define SEQ_MAX      (NRF_FICR->CODEPAGESIZE * 8 * SEQ_PER_BIT)

static struct device *gpio;
static struct device *nvm;
static struct device *pwm;

static struct k_work button_work;

static struct k_work attention_work;
static bool attention;
//#define BEACON_STACK_SIZE 1024
//K_THREAD_STACK_DEFINE(attention_stack_area, BEACON_STACK_SIZE);
//struct k_thread attention_thread_data;

static void button_send_pressed(struct k_work *work)
{
	printk("button_send_pressed()\n");
	board_button_1_pressed();
}

static void button_pressed(struct device *dev, struct gpio_callback *cb,
			   u32_t pins)
{
	struct mb_display *disp = mb_display_get();

	//k_sleep(100); //trying to manually debounce...doesnt work as the interrupts still get triggered

	if (pins & BIT(SW0_GPIO_PIN)) {
		k_work_submit(&button_work);
	} else {
		u16_t target = board_set_experiment();

		/*if (target > 0x0008) {
			mb_display_print(disp, MB_DISPLAY_MODE_SINGLE,
					 K_SECONDS(2), "A");
		} else {
			mb_display_print(disp, MB_DISPLAY_MODE_SINGLE | MB_DISPLAY_FLAG_LOOP, K_SECONDS(2), "%X", (target & 0xf));
		}*/
		mb_display_print(disp, MB_DISPLAY_MODE_SINGLE | MB_DISPLAY_FLAG_LOOP, K_SECONDS(2), "%X", (target & 0xf));
	}
}

static const struct {
	char  note;
	u32_t period;
	u32_t sharp;
} period_map[] = {
	{ 'C',  3822,  3608 },
	{ 'D',  3405,  3214 },
	{ 'E',  3034,  3034 },
	{ 'F',  2863,  2703 },
	{ 'G',  2551,  2407 },
	{ 'A',  2273,  2145 },
	{ 'B',  2025,  2025 },
};

static u32_t get_period(char note, bool sharp)
{
	int i;

	if (note == ' ') {
		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(period_map); i++) {
		if (period_map[i].note != note) {
			continue;
		}

		if (sharp) {
			return period_map[i].sharp;
		} else {
			return period_map[i].period;
		}
	}

	return 1500;
}

void board_play_tune(const char *str)
{
	while (*str) {
		u32_t period, duration = 0U;

		while (*str && !isdigit((unsigned char)*str)) {
			str++;
		}

		while (isdigit((unsigned char)*str)) {
			duration *= 10U;
			duration += *str - '0';
			str++;
		}

		if (!*str) {
			break;
		}

		if (str[1] == '#') {
			period = get_period(*str, true);
			str += 2;
		} else {
			period = get_period(*str, false);
			str++;
		}

		if (period) {
			pwm_pin_set_usec(pwm, BUZZER_PIN, period, period / 2U);
		}

		k_sleep(duration);

		/* Disable the PWM */
		pwm_pin_set_usec(pwm, BUZZER_PIN, 0, 0);
	}
}

void board_heartbeat(u8_t hops, u16_t feat)
{
	struct mb_display *disp = mb_display_get();
	const struct mb_image hops_img[] = {
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 0, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 0, 1, 0, 1 },
			 { 0, 0, 0, 0, 0 },
			 { 1, 0, 0, 0, 1 },
			 { 0, 0, 0, 0, 0 },
			 { 1, 0, 1, 0, 1 })
	};

	printk("%u hops\n", hops);

	if (hops) {
		hops = MIN(hops, ARRAY_SIZE(hops_img));
		mb_display_image(disp, MB_DISPLAY_MODE_SINGLE, K_SECONDS(2),
				 &hops_img[hops - 1], 1);
	}
}

void board_other_dev_pressed(u16_t addr)
{
	struct mb_display *disp = mb_display_get();

	printk("board_other_dev_pressed(0x%04x)\n", addr);

	mb_display_print(disp, MB_DISPLAY_MODE_SINGLE, K_SECONDS(2),
			 "%X", (addr & 0xf));
}

/*void attention_entry_point(void *unused1, void *unused2, void *unused3) {
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    printk("Inside attetion thread\n");

    while (1) {
        send_data();
        k_sleep(K_MSEC(400));
    }
}
*/

static void attention_callback(struct k_work *work){
	struct mb_display *disp = mb_display_get();
	static const struct mb_image attn_img[] = {
		MB_IMAGE({ 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 },
			 { 0, 0, 1, 0, 0 },
			 { 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 }),
		MB_IMAGE({ 0, 0, 0, 0, 0 },
			 { 0, 1, 1, 1, 0 },
			 { 0, 1, 1, 1, 0 },
			 { 0, 1, 1, 1, 0 },
			 { 0, 0, 0, 0, 0 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 0, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 1, 1, 1, 1 }),
	};

	if (attention) {
		mb_display_image(disp,
				 MB_DISPLAY_MODE_DEFAULT | MB_DISPLAY_FLAG_LOOP,
				 K_MSEC(150), attn_img, ARRAY_SIZE(attn_img));
	} else {
		mb_display_stop(disp);
	}
}

void board_attention(bool x) {
	//add the bool attention to the attention_work
	attention = x;

	k_work_submit(&attention_work);
}

static void configure_button(void)
{
	static struct gpio_callback button_cb;

	k_work_init(&button_work, button_send_pressed);

	gpio = device_get_binding(SW0_GPIO_CONTROLLER);

	//Debounce not really working...
	gpio_pin_configure(gpio, SW0_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE));
	gpio_pin_configure(gpio, SW1_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE));

	gpio_init_callback(&button_cb, button_pressed,
			   BIT(SW0_GPIO_PIN) | BIT(SW1_GPIO_PIN));
	gpio_add_callback(gpio, &button_cb);

	gpio_pin_enable_callback(gpio, SW0_GPIO_PIN);
	gpio_pin_enable_callback(gpio, SW1_GPIO_PIN);
}

void board_init(u16_t *addr)
{
	struct mb_display *disp = mb_display_get();

	nvm = device_get_binding(DT_FLASH_DEV_NAME);
	pwm = device_get_binding(CONFIG_PWM_NRF5_SW_0_DEV_NAME);

	*addr = NRF_UICR->CUSTOMER[0];
	if (!*addr || *addr == 0xffff) {
#if defined(NODE_ADDR)
		*addr = NODE_ADDR;
#else
		*addr = 0x0b0c;
#endif
	}

	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT, SCROLL_SPEED,
			 "0x%04x", *addr);

	configure_button();

	/*k_tid_t attention_thread_id = k_thread_create(&attention_thread_data,
                                    attention_stack_area,
                                    K_THREAD_STACK_SIZEOF(beacon_stack_area),
                                    attention_entry_point,
                                    NULL, NULL, NULL, -5, 0, K_NO_WAIT);
    printk("Created attention thread with id %p\n", attention_thread_id);
    */

	k_work_init(&attention_work, attention_callback);

}
