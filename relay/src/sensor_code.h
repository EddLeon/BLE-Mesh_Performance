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
// - Sensor Client SIG 		-> 0x1102
//--------------
void sensor_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	printk("sensor_get\n");
}

void sensor_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	printk("sensor_status\n");
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
const struct bt_mesh_model_op sensor_srv_op[] = {
	{BT_MESH_MODEL_OP_SENSOR_GET, 0, sensor_get},
	{BT_MESH_MODEL_OP_SENSOR_STATUS, 0, sensor_status},
	{BT_MESH_MODEL_OP_SENSOR_COLUMN_GET, 2, sensor_column_get},
	{BT_MESH_MODEL_OP_SENSOR_COLUMN_STATUS, 2, sensor_column_status},
	{BT_MESH_MODEL_OP_SENSOR_SERIES_GET, 2, sensor_series_get},
	{BT_MESH_MODEL_OP_SENSOR_SERIES_STATUS, 2, sensor_series_status},
	BT_MESH_MODEL_OP_END,
};

// Ref 4.2.2 in the spec "Model Publication"
//struct bt_mesh_model_pub sensor_srv_pub;
