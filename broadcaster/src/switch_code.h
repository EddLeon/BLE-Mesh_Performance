// --------------------
// Generic OnOff Server
// --------------------

// message opcodes
#define BT_MESH_MODEL_OP_GENERIC_ONOFF_GET BT_MESH_MODEL_OP_2(0x82, 0x01)
#define BT_MESH_MODEL_OP_GENERIC_ONOFF_SET BT_MESH_MODEL_OP_2(0x82, 0x02)
#define BT_MESH_MODEL_OP_GENERIC_ONOFF_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x03)
#define BT_MESH_MODEL_OP_GENERIC_ONOFF_STATUS BT_MESH_MODEL_OP_2(0x82, 0x04) 

void onoff_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	printk("onoff_get\n");
}
void onoff_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	printk("onoff_set\n");
}
void onoff_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	printk("onoff_set_unack\n");
}

// OnOff messages
const struct bt_mesh_model_op generic_onoff_op[] = {
	{BT_MESH_MODEL_OP_GENERIC_ONOFF_GET, 0 onoff_get},
	{BT_MESH_MODEL_OP_GENERIC_ONOFF_SET, 2, onoff_set},
	{BT_MESH_MODEL_OP_GENERIC_ONOFF_SET_UNACK, 2, onoff_set_unack},
	BT_MESH_MODEL_OP_END,
};
