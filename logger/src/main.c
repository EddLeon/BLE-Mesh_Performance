/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/printk.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#include "board.h"
#include "sensor_code.h"

#define MOD_LF 0x0000

#define GROUP_ADDR 0xc000
#define PUBLISHER_ADDR  0x000f

#define OP_VENDOR_BUTTON BT_MESH_MODEL_OP_3(0x00, BT_COMP_ID_LF)

static const u8_t net_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u8_t dev_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u8_t app_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u16_t net_idx;
static const u16_t app_idx;
static const u32_t iv_index;
static u8_t flags;
static u16_t addr = NODE_ADDR;

static void heartbeat(u8_t hops, u16_t feat)
{
	board_heartbeat(hops, feat);
	board_play("100H");
}

static struct bt_mesh_cfg_srv cfg_srv = {
#if defined(CONFIG_BOARD_BBC_MICROBIT)
	.relay = BT_MESH_RELAY_ENABLED,
	.beacon = BT_MESH_BEACON_DISABLED,
#else
	.relay = BT_MESH_RELAY_ENABLED,
	.beacon = BT_MESH_BEACON_ENABLED,
#endif
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
	.default_ttl = 7,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(3, 20),

	.hb_sub.func = heartbeat,
};

static struct bt_mesh_cfg_cli cfg_cli = {
};

static void attention_on(struct bt_mesh_model *model)
{
	printk("attention_on()\n");
	board_attention(true);
	board_play("100H100C100H100C100H100C");
}

static void attention_off(struct bt_mesh_model *model)
{
	printk("attention_off()\n");
	board_attention(false);
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_SENSOR_CLI, sensor_cli_op, NULL, NULL),
};

static void vnd_button_pressed(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	printk("src 0x%04x\n", ctx->addr);

	if (ctx->addr == bt_mesh_model_elem(model)->addr) {
		return;
	}

	board_other_dev_pressed(ctx->addr);
	board_play("100G200 100G");
}

static const struct bt_mesh_model_op vnd_ops[] = {
	{ OP_VENDOR_BUTTON, 0, vnd_button_pressed },
	BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_model vnd_models[] = {
	BT_MESH_MODEL_VND(BT_COMP_ID_LF, MOD_LF, vnd_ops, NULL, NULL),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, vnd_models),
};

static const struct bt_mesh_comp comp = {
	.cid = BT_COMP_ID_LF,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

static void configure(void)
{
	printk("Configuring...\n");

	/* Add Application Key */
	bt_mesh_cfg_app_key_add(net_idx, addr, net_idx, app_idx, app_key, NULL);

	/* Bind to vendor model */
	bt_mesh_cfg_mod_app_bind_vnd(net_idx, addr, addr, app_idx,
				     MOD_LF, BT_COMP_ID_LF, NULL);

	/* Bind to sensor client model */
	bt_mesh_cfg_mod_app_bind(net_idx, addr, addr, app_idx,
				    BT_MESH_MODEL_ID_SENSOR_CLI, NULL);

	/* Bind to Health model */
	bt_mesh_cfg_mod_app_bind(net_idx, addr, addr, app_idx,
				 BT_MESH_MODEL_ID_HEALTH_SRV, NULL);

	/* Add model subscription */
	bt_mesh_cfg_mod_sub_add_vnd(net_idx, addr, addr, GROUP_ADDR,
				    MOD_LF, BT_COMP_ID_LF, NULL);
	
	/* Add sensor model subscription */ 
	bt_mesh_cfg_mod_sub_add(net_idx,addr,addr,GROUP_ADDR,BT_MESH_MODEL_ID_SENSOR_CLI,NULL);

#if NODE_ADDR == PUBLISHER_ADDR
	{
		struct bt_mesh_cfg_hb_pub pub = {
			.dst = GROUP_ADDR,
			.count = 0xff,
			.period = 0x05,
			.ttl = 0x07,
			.feat = 0,
			.net_idx = net_idx,
		};

		bt_mesh_cfg_hb_pub_set(net_idx, addr, &pub, NULL);
		printk("Publishing heartbeat messages\n");
	}
#endif
	printk("Configuration complete\n");
	board_play("100C100D100E100F100G100A100H");
}

static const u8_t dev_uuid[16] = { 0xdd, 0xdd };

static const struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		printk("Initializing mesh failed (err %d)\n", err);
		return;
	}

	printk("Mesh initialized\n");

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		printk("Loading stored settings\n");
		settings_load();
	}

	err = bt_mesh_provision(net_key, net_idx, flags, iv_index, addr,
				dev_key);
	if (err == -EALREADY) {
		printk("Using stored settings\n");
	} else if (err) {
		printk("Provisioning failed (err %d)\n", err);
		return;
	} else {
		printk("Provisioning completed\n");
		configure();
	}

#if NODE_ADDR != PUBLISHER_ADDR
	/* Heartbeat subcscription is a temporary state (due to there
	 * not being an "indefinite" value for the period, so it never
	 * gets stored persistently. Therefore, we always have to configure
	 * it explicitly.
	 */
	{
		struct bt_mesh_cfg_hb_sub sub = {
			.src = PUBLISHER_ADDR,
			.dst = GROUP_ADDR,
			.period = 0x10,
		};

		bt_mesh_cfg_hb_sub_set(net_idx, addr, &sub, NULL);
		printk("Subscribing to heartbeat messages\n");
	}
#endif
}

static u16_t experiment_no = 1;

void board_button_1_pressed(void)
{
	NET_BUF_SIMPLE_DEFINE(msg, 3 + 8 + 4); //initializing buffer

	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = app_idx,
		.addr = GROUP_ADDR,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};

	/* Bind to Vendor Button model opperation*/
	bt_mesh_model_msg_init(&msg, OP_VENDOR_BUTTON);

	net_buf_simple_add_u8(&msg, experiment_no); //putting the experimetn number
	if (bt_mesh_model_send(&vnd_models[0], &ctx, &msg, NULL, NULL)) {
		printk("Unable to send Vendor Button message\n");
	}

	printk("Button message sent with OpCode 0x%08x\n", OP_VENDOR_BUTTON);
}

u16_t board_set_experiment(void)
{
	/*switch (experiment_no) {
	case 9:
		experiment_no = 0U;
		break;
	default:
		experiment_no++;
		break;
	}*/

	if(experiment_no == 8)
		experiment_no = 0U;
	else experiment_no++;

	return experiment_no;
}

static K_SEM_DEFINE(tune_sem, 0, 1);
static const char *tune_str;

void board_play(const char *str)
{
	tune_str = str;
	k_sem_give(&tune_sem);
}

void main(void)
{
	int err;

	printk("Initializing...\n");

	board_init(&addr);

	printk("Unicast address: 0x%04x\n", addr);

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	printk("SENSOR CLIENT\n");

	while (1) {
		k_sem_take(&tune_sem, K_FOREVER);
		board_play_tune(tune_str);
	}

}
