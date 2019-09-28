#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

//Message OP-Codes (7.1-Message Summary table - MeshModel) 
#define BT_MESH_MODEL_OP_SENSOR_GET BT_MESH_MODEL_OP_2(0x82, 0x31)
#define BT_MESH_MODEL_OP_SENSOR_STATUS BT_MESH_MODEL_OP_2(0x00, 0x52)
#define BT_MESH_MODEL_OP_SENSOR_COLUMN_GET BT_MESH_MODEL_OP_2(0x82, 0x32)
#define BT_MESH_MODEL_OP_SENSOR_COLUMN_STATUS BT_MESH_MODEL_OP_2(0x00, 0x53)
#define BT_MESH_MODEL_OP_SENSOR_SERIES_GET BT_MESH_MODEL_OP_2(0x82, 0x33)
#define BT_MESH_MODEL_OP_SENSOR_SERIES_STATUS BT_MESH_MODEL_OP_2(0x00, 0x54)

//--------------
//Sensor Model (7.3 Model Summary)
// - Sensor Server SIG 		-> 0x1100 BT_MESH_MODEL_ID_SENSOR_SRV  (defined in access.h)              
// - Sensor Setup Server SIG 	-> 0x1101
// - Sensor Client SIG 		-> 0x1102 BT_MESH_MODEL_ID_GEN_ONOFF_CLI
//--------------
void sensor_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	printk("sensor_get\n");
	printk("buf lenght: 0x%04x", buf->len);
	printk("buf data[0]: 0x%04x", buf->data[0]);

}
/*
 * This is the callback which is used by the Sensor-Client model to receive sensor data
 */
void sensor_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	//this will be used to capture traffic through serial so it can be logged
	//printk("%04x,%04x,%i,%i,%02x,%i\n",ctx->addr,ctx->recv_dst,ctx->recv_rssi,ctx->recv_ttl,buf->len, (buf->data[1]<<8)|buf->data[0]);
	printk("%04x,%04x,%i,%02x,%i\n",ctx->addr,ctx->recv_dst,ctx->recv_ttl,buf->len, (buf->data[1]<<8)|buf->data[0]);
}

void sensor_column_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	printk("sensor_column_get\n");
}

void sensor_column_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	printk("sensor_column_status\n");
}

void sensor_series_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	printk("sensor_series_get\n");
}
void sensor_series_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	printk("sensor_series_status\n");
}

//4.2 Sensor messages
//Binding OpCodes to function callbacks
const struct bt_mesh_model_op sensor_srv_op[] = {
	{BT_MESH_MODEL_OP_SENSOR_GET, 0, sensor_get},
	{BT_MESH_MODEL_OP_SENSOR_STATUS, 0, sensor_status},
	{BT_MESH_MODEL_OP_SENSOR_COLUMN_GET, 2, sensor_column_get},
	{BT_MESH_MODEL_OP_SENSOR_COLUMN_STATUS, 2, sensor_column_status},
	{BT_MESH_MODEL_OP_SENSOR_SERIES_GET, 2, sensor_series_get},
	{BT_MESH_MODEL_OP_SENSOR_SERIES_STATUS, 2, sensor_series_status},
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op sensor_cli_op[] = {
	{BT_MESH_MODEL_OP_SENSOR_GET, 0, sensor_get},
	{BT_MESH_MODEL_OP_SENSOR_STATUS, 0, sensor_status},
 	{BT_MESH_MODEL_OP_SENSOR_COLUMN_GET, 2, sensor_column_get},
	{BT_MESH_MODEL_OP_SENSOR_COLUMN_STATUS, 2, sensor_column_status},
	{BT_MESH_MODEL_OP_SENSOR_SERIES_GET, 2, sensor_series_get},
	{BT_MESH_MODEL_OP_SENSOR_SERIES_STATUS, 2, sensor_series_status},
	BT_MESH_MODEL_OP_END,
};

// Ref 4.2.2 in the spec "Model Publication"
struct bt_mesh_model_pub sensor_srv_pub;
