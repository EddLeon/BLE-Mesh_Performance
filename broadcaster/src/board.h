/* board.h - Board-specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(NODE_ADDR)
#define NODE_ADDR 0x0001
#endif

#define EXP_STATUS_OFF 0x00
#define EXP_STATUS_WARNING 0xff

void board_button_1_pressed(void);
//u16_t board_set_experiment(void);
void board_play(const char *str);

#if defined(CONFIG_BOARD_BBC_MICROBIT)
void board_init(u16_t *addr);
void board_play_tune(const char *str);
void board_heartbeat(u8_t hops, u16_t feat);
void board_other_dev_pressed(u16_t addr);
void board_attention(u8_t attention);
void attention_callback(struct k_work *work);
#else
static inline void board_init(u16_t *addr)
{
	*addr = NODE_ADDR;
}

static inline void board_play_tune(const char *str)
{
}

void board_heartbeat(u8_t hops, u16_t feat)
{
}

void board_other_dev_pressed(u16_t addr)
{
}

void board_attention(bool attention)
{
}
static void attention_callback(struct k_work *work);
#endif
