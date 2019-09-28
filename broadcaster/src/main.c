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
//#include "switch_code.h"

#define MOD_LF 0x0000

#define GROUP_ADDR 0xc000
#define HEALTH_GROUP 0xc001
#define PAYLOAD_OCTETS	0x04//0x07

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
u16_t app_idx;//static const 
static const u32_t iv_index;
static u8_t flags;
static u16_t addr = NODE_ADDR;

static K_SEM_DEFINE(off_flag_sem, 0, 1);// semaphore to avoid race conditions on flag
static bool off_flag = false;// used to check if an OFF command has been issued by the logger on the publishing of the broadcaster
struct publish_work_wrapper{
	struct k_delayed_work publish_cfg_handler;
	bool attention;
	s32_t rand_delay;
	u8_t period;
}publish_work;

static void heartbeat(u8_t hops, u16_t feat)
{
	board_heartbeat(hops, feat);
	board_play("100H");
}

static struct bt_mesh_cfg_srv cfg_srv = {
#if defined(CONFIG_BOARD_BBC_MICROBIT)
	//.relay = BT_MESH_RELAY_DISABLED,
	.beacon = BT_MESH_BEACON_DISABLED,
#else
	//.relay = BT_MESH_RELAY_ENABLED,
	.beacon = BT_MESH_BEACON_ENABLED,
#endif
	.relay = BT_MESH_RELAY_NOT_SUPPORTED,
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
	.default_ttl = 0,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	//.relay_retransmit = BT_MESH_TRANSMIT(3, 20),

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


/*
 * Callback function for UPDATING payload of the periodic publising 
 */
static int sensor_pub_update(struct bt_mesh_model *mod)
{
       	struct net_buf_simple *msg = mod->pub->msg;
	static int packet_count;

       	printk("%i\n",packet_count);

	/*
	 * HOTFIX mechanism to detect that publishing hasn't really stopped
	 */
	k_sem_take(&off_flag_sem, K_FOREVER);
	if(off_flag){
		k_sem_give(&off_flag_sem);
		board_attention(EXP_STATUS_WARNING); //send a warning code to the board
		goto skip_update;
	}
	else
		k_sem_give(&off_flag_sem);

       	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_SENSOR_STATUS);
	int i;

	// filling up the payload and puting the current broadcast 
	// counter inside
 	for(i=0; i<PAYLOAD_OCTETS; i++){
		net_buf_simple_add_le16(msg, packet_count);//2 bytes addition to payload
       }

	packet_count++; 

skip_update:

       return 0;
}

BT_MESH_MODEL_PUB_DEFINE(sensor_srv_pub, sensor_pub_update, 3 + 1);

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_SENSOR_SRV, sensor_srv_op, &sensor_srv_pub, NULL),
};

/*
 *  This Callback function Configures the Publishing of the Sensor Server Model
 *  Must be done on a separate thread to avoid blocking the ISR
 *  
 *  It runs on the System's workqueue
 */
static void publish_cfg_hcallback(struct k_work *work){

	struct publish_work_wrapper *wrapper = CONTAINER_OF(work, struct publish_work_wrapper, publish_cfg_handler);

	// waiting for a random delay before activating the publishing
	//k_sleep(wrapper->rand_delay); //no need anymore....

	struct bt_mesh_cfg_mod_pub pub_sensor = {
			.addr = GROUP_ADDR,
			.app_idx = app_idx,
			.ttl = 0x04,
			.period = wrapper->period,
		};
	bt_mesh_cfg_mod_pub_set(net_idx, addr, addr, BT_MESH_MODEL_ID_SENSOR_SRV, &pub_sensor, NULL);

	printk("publish cfg success period: 0x%04x\n", pub_sensor.period);
	if(pub_sensor.period == 0x40){
		board_attention(EXP_STATUS_OFF);
	}
}
/*
 * This Callback function is triggered with the Broadaster node
 *  receives a message from the Logger node requesting to change
 *  the publishing period
 */
static void vnd_button_pressed(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	u8_t period = 0;
	s32_t delay = 0;
	u32_t rand = sys_rand32_get();// & 0x7fffffff;//masking the sign bit
	printk("src 0x%04x length: 0x%04x  exp# 0x%04x  rand: %i\n", ctx->addr,buf->len, buf->data[0], rand);

	switch (buf->data[0]){
		case 0:	//turn off the publishing
			printk("case 0 \n");
			period = BT_MESH_PUB_PERIOD_SEC(0); 
			
			//crit section
			k_sem_take(&off_flag_sem, K_FOREVER);
			off_flag = true;
			k_sem_give(&off_flag_sem);
			//out of crit section
			
			goto skip_flag;
			break;
		case 1:
			printk("case 1 \n");
			period = BT_MESH_PUB_PERIOD_SEC(30); 
			delay = rand%30000;
			break;
		case 2: 
			printk("case 2 \n");
			period = BT_MESH_PUB_PERIOD_SEC(10); 
			delay = rand%10000;
			break;
		case 3: 
			printk("case 3 \n");
			//period = BT_MESH_PUB_PERIOD_SEC(5); 
			period = BT_MESH_PUB_PERIOD_100MS(2); //For the nomadic nodes
			//period = BT_MESH_PUB_PERIOD_SEC(1); 
			delay = rand%10000;
			break;
		case 4: 
			printk("case 4 \n");
			period = BT_MESH_PUB_PERIOD_SEC(1); 
			delay = rand%1000;
			break;
		case 5:
			printk("case 5 \n");
			period = BT_MESH_PUB_PERIOD_100MS(5); 
			delay = rand%500;
			break;
		case 6:
			printk("case 6 \n");
			period = BT_MESH_PUB_PERIOD_100MS(3); 
			delay = rand%300;
			break;
		case 7:
			printk("case 7 \n");
			period = BT_MESH_PUB_PERIOD_100MS(2); 
			delay = rand%200;
			break;
		case 8:
			printk("case 8 \n");
			period = BT_MESH_PUB_PERIOD_100MS(1); 
			delay = rand%100;
			break;
		default:
			printk("invalud Command: 0x%04x\n",buf->data[0]);
			return;
			break;
	}

	//crit section
	k_sem_take(&off_flag_sem, K_FOREVER);
	off_flag = false;
	k_sem_give(&off_flag_sem);
	//out of crit section

skip_flag:
	printk("Random Delay: %i\n",delay);
	publish_work.period = period;
	publish_work.rand_delay = delay;
	// Sumbitting the Publishing configuration on the system queue 
	// as a workitem
	k_delayed_work_submit(&publish_work.publish_cfg_handler, delay);

	//Activating led matrix to indicate Broadcasting on/off
	if(buf->data[0])
		board_attention(buf->data[0]);

	return;
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

	/* Bind to sensor server model */
	bt_mesh_cfg_mod_app_bind(net_idx, addr, addr, app_idx,
				    BT_MESH_MODEL_ID_SENSOR_SRV, NULL);

	/* Bind to Health model */
	bt_mesh_cfg_mod_app_bind(net_idx, addr, addr, app_idx,
				 BT_MESH_MODEL_ID_HEALTH_SRV, NULL);

	/* Add model subscription */
	bt_mesh_cfg_mod_sub_add_vnd(net_idx, addr, addr, GROUP_ADDR,
				    MOD_LF, BT_COMP_ID_LF, NULL);

	/* Add sensor model subscription */
	bt_mesh_cfg_mod_sub_add(net_idx, addr, addr, GROUP_ADDR, BT_MESH_MODEL_ID_HEALTH_SRV, NULL);

	struct bt_mesh_cfg_hb_pub pub = {
		.dst = GROUP_ADDR,
		.count = 0xff,
		.period = 0x05,
		.ttl = 0x07,
		.feat = 0,
		.net_idx = net_idx,
	};

		//printk("Publishing heartbeat messages\n");
		//bt_mesh_cfg_hb_pub_set(net_idx, addr, &pub, NULL);

		
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
}

//static u16_t experiment_no = 1;

void board_button_1_pressed(void)
{
	/*NET_BUF_SIMPLE_DEFINE(msg, 3 + 4); //initializing buffer
	net_buf_simple_add_u8(&msg, experiment_no); //putting the experimetn number

	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = app_idx,
		.addr = GROUP_ADDR,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	*/
	/* Bind to Vendor Button model opperation*/
	/*bt_mesh_model_msg_init(&msg, OP_VENDOR_BUTTON);

	if (bt_mesh_model_send(&vnd_models[0], &ctx, &msg, NULL, NULL)) {
		printk("Unable to send Vendor Button message\n");
	}

	printk("Button message sent with OpCode 0x%08x\n", OP_VENDOR_BUTTON);
	*/
}

/*u16_t board_set_experiment(void)
{
	switch (experiment_no) {
	case 5:
		experiment_no = 1U;
		break;
	default:
		experiment_no++;
		break;
	}

	return experiment_no;
}*/

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

	//initialize the semaphore of the off_flag	
	k_sem_give(&off_flag_sem);

	// Initializing the Work handler to start Pubishing
	k_delayed_work_init(&publish_work.publish_cfg_handler, publish_cfg_hcallback);
	printk("Unicast address: 0x%04x\n", addr);

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	while (1) {
		k_sem_take(&tune_sem, K_FOREVER);
		board_play_tune(tune_str);
	}

}
