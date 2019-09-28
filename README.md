# BLE-Mesh Performance Evaluation Project

This repository contains the firmware used to program [Micro:bit](https://microbit.org/) boards for the BLE-Mesh project. In the different folders, you will find the corresponding code for the different type of nodes used for the experiments:
- **Loggger/Control**: Contains the firmware for the *base-station* node which is in charge of starting the experiments and logging the traffic comming from other nodes. This code contains a SensorClient and a custom VendorServer model.
- **Broadcaster**: Contains the firmware of the generic sensor nodes (traffic generators). This nodes will listen to the commands sent by the *base station* and start publishing their packets. A good future implementation for this nodes would be actually sending sensor data from their sensors as the communication base has already been programmed into them.
- **Relay**: This folder contains the firmware for the relay nodes. The relays do not have any other task other than *relaying*; special focus on the kernel configurations for proper buffer sizes and such have been done for these nodes (see ).

This firmware was developed using the Official SIG Educational KIT: [An Introduction to Bluetooth Mesh Networking](https://www.bluetooth.com/bluetooth-resources/bluetooth-mesh-developer-study-guide/) and Zephyr's samples.

To measure performance, the **broadcasters** send unsegmented 8 byte payload packets containing a message counter. The counter is then used calculate performance metrics such as Packet-Delivery-Ratio (PDR), Burst-Drops (BD)...This packets may be *logged* by any **logger** node subscribed to the publishing address.

## Usage
---
1. **Setup** [Zephyr RTOS](https://docs.zephyrproject.org/latest/getting_started/index.html)
    - [Zephyr 1.14.0](https://github.com/zephyrproject-rtos/zephyr/releases/tag/zephyr-v1.14.0)
    - [SDK v0.10.0](https://github.com/zephyrproject-rtos/sdk-ng/releases/tag/v0.10.0)
2. **[Build](https://docs.zephyrproject.org/latest/application/index.html#build-an-application)** using CMake and ninja
3. **Flash** using `ninja flash` (you might need pyocd [udev rules](https://github.com/mbedmicro/pyOCD/tree/master/udev))


#### Zephyr API Examples
* **Message sending**
    ```
    //Vendor Model Operation (Custom model)
    #define OP_VENDOR_BUTTON BT_MESH_MODEL_OP_3(0x00, BT_COMP_ID_LF)
    
    void start_experiment_control(void)
    {
    	/* Defining the TX Buffer
    	 *  3-byte Vendor-Opcode
    	 *  8-byte Payload (Experiment Number)
    	 *  4-byte Transport MIC (message integrity check)
    	 */
    	NET_BUF_SIMPLE_DEFINE(msg, 3 + 8 + 4);
    
    	//Setting up Message context
    	struct bt_mesh_msg_ctx ctx = {
    		.net_idx = net_idx, //Net-Key index
    		.app_idx = app_idx, //App-Key index
    		.addr = GROUP_ADDR, //Destination
    		.send_ttl = BT_MESH_TTL_DEFAULT,
    	};
    
    	// Initializing message and binding to OpCode
    	bt_mesh_model_msg_init(&msg, OP_VENDOR_BUTTON);
    
    	net_buf_simple_add_u8(&msg, experiment_no); //Putting the experiment number
    	if (bt_mesh_model_send(&vnd_models[0], &ctx, &msg, NULL, NULL)) {
    		printk("Unable to send Vendor Button message\n");
    		return;
    	}
    	printk("Message sent with OpCode 0x%08x\n", OP_VENDOR_BUTTON);
    }
    ```
* **Model definition and handling**
    ```
    /* Refer to include/bluetooth/mesh/access.h */

    /*OpCode Definitions*/
    //MeshModel | 7.1-Message Summary table
    #define BT_MESH_MODEL_OP_SENSOR_GET BT_MESH_MODEL_OP_2(0x82, 0x31)
    #define BT_MESH_MODEL_OP_SENSOR_STATUS BT_MESH_MODEL_OP_1(0x52)
    #define BT_MESH_MODEL_OP_SENSOR_COLUMN_GET BT_MESH_MODEL_OP_2(0x82, 0x32)
    #define BT_MESH_MODEL_OP_SENSOR_COLUMN_STATUS BT_MESH_MODEL_OP_1(0x53)
    #define BT_MESH_MODEL_OP_SENSOR_SERIES_GET BT_MESH_MODEL_OP_2(0x82, 0x33)
    #define BT_MESH_MODEL_OP_SENSOR_SERIES_STATUS BT_MESH_MODEL_OP_1(0x54)
    
    /*Mapping OpCodes to the Handling Function*/
    //Mesh Model | 4.2 Sensor messages
    const struct bt_mesh_model_op sensor_srv_op[] = {
    //{OpCode, min req. length, handler function}
    	{BT_MESH_MODEL_OP_SENSOR_GET, 0, sensor_get},
    	{BT_MESH_MODEL_OP_SENSOR_STATUS, 0, sensor_status},
    	{BT_MESH_MODEL_OP_SENSOR_COLUMN_GET, 2, sensor_column_get},
    	{BT_MESH_MODEL_OP_SENSOR_COLUMN_STATUS, 2, sensor_column_status},
    	{BT_MESH_MODEL_OP_SENSOR_SERIES_GET, 2, sensor_series_get},
    	{BT_MESH_MODEL_OP_SENSOR_SERIES_STATUS, 2, sensor_series_status},
    	BT_MESH_MODEL_OP_END,
    };
    ```
* **Node composition**
    ```
    //The CONFIGURATION SERVER
    static struct bt_mesh_cfg_srv cfg_srv = {
    	//Node features
    	.relay = BT_MESH_RELAY_ENABLED,
    	.beacon = BT_MESH_BEACON_DISABLED,
    	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
    	.default_ttl = 4,
    
    	//Transmission count and Interval
    	.net_transmit = BT_MESH_TRANSMIT(2, 20),
    	.relay_retransmit = BT_MESH_TRANSMIT(2, 20),
    
    	// Heartbeat callback function
    	.hb_sub.func = heartbeat,
    };
    
    //Model Publisher Definition, its callback and message length
    BT_MESH_MODEL_PUB_DEFINE(sensor_srv_pub, sensor_pub_update, 3 + 1);
    
    // Root Models 
    static struct bt_mesh_model root_models[] = {
    	BT_MESH_MODEL_CFG_SRV(&cfg_srv), //Configuration Server Model
    	BT_MESH_MODEL_CFG_CLI(&cfg_cli), //Configuration Client Model
    	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub), //Health Server Model
    	BT_MESH_MODEL(BT_MESH_MODEL_ID_SENSOR_SRV, sensor_srv_op, &sensor_srv_pub, NULL), //Sensor Server Model definition
    };
    
    // Vendor Models
    static struct bt_mesh_model vnd_models[] = {
    	BT_MESH_MODEL_VND(BT_COMP_ID_LF, MOD_LF, vnd_ops, NULL, NULL), //Controller Model
    };
    
    // Element Array
    static struct bt_mesh_elem elements[] = {
    	//Primary Element
    	BT_MESH_ELEM(0, root_models, vnd_models), //Asigning the Models to Element
    };
    
    // Full Node Composition
    static const struct bt_mesh_comp comp = {
    	.cid = BT_COMP_ID_LF, //Company ID (BT assigned #s)
    	.elem = elements,
    	.elem_count = ARRAY_SIZE(elements),
    };
    ```
* **Model publication configuration**
    ```
    /* Model Publishing Callback function
     * to update @ref bt_mesh_model_pub.msg 
     * with a valid publication message
     */
    static int sensor_pub_update(struct bt_mesh_model *mod){
    	// Pointer to the buffer that will be sent
    	struct net_buf_simple *msg = mod->pub->msg;
    	//No need to send the message here, It will automatically
    	//be sent by the publishing mechanism
    }
    
    BT_MESH_MODEL_PUB_DEFINE(sensor_srv_pub, sensor_pub_update, 3 + 1);
    
    struct bt_mesh_cfg_mod_pub pub 	= {
    	.addr = GROUP_ADDR,
    	.app_idx = APP_IDX,
    	.ttl = DEFAULT_TTL,
    	.period = BT_MESH_PUB_PERIOD_SEC(5),
    };
    
    /* Refer to subsys/bluetooth/host/mesh/cfg_cli.c 
     *
     * After this function is called, the "sensor_pub_update" function will start 
     * getting periodically triggered
     * Params:
     *	   NetKey Index
     *	   Node Address
     *	   Element Address
     *	   Publishing Configuration
     *	   Status
     */
    bt_mesh_cfg_mod_pub_set(NET_IDX, NODE_ADDR, NODE_ADDR, BT_MESH_MODEL_ID_SENSOR_SRV,  &pub, NULL);
    ```
#### Other useful examples
Official SIG Educational KIT: [An Introduction to Bluetooth Mesh Networking](https://www.bluetooth.com/bluetooth-resources/bluetooth-mesh-developer-study-guide/)

Martin Woolley's presentation: [BLE-Mesh and Zephyr](https://events.linuxfoundation.org/wp-content/uploads/2017/12/Bluetooth-Mesh-and-Zephyr-V1.0_Martin-Wooley.pdf)

Zephyr Samples:
* [BLE-Mesh example](https://github.com/zephyrproject-rtos/zephyr/tree/v1.14-branch/samples/bluetooth/mesh)
* [BLE-Mesh demo](https://github.com/zephyrproject-rtos/zephyr/tree/v1.14-branch/samples/bluetooth/mesh_demo)
* [Other Micro:bit examples](https://github.com/zephyrproject-rtos/zephyr/tree/v1.14-branch/samples/boards/bbc_microbit)
## Useful Links
---
#### [Micro:bit](https://microbit.org/)
* micro:bit [software](https://tech.microbit.org/software/)
* micro:bit [hardware](https://tech.microbit.org/hardware/)
* Other projects [ideas](https://microbit.org/ideas/)
* Google Play  [app](https://play.google.com/store/apps/details?id=com.samsung.microbit)
#### [Zephyr](https://www.zephyrproject.org/)
* [Zephyr Documentation](https://docs.zephyrproject.org/latest/getting_started/index.html)
* [Micro:bit board](https://docs.zephyrproject.org/latest/boards/arm/bbc_microbit/doc/index.html)
* [Zephyr Slack](https://zephyrproject.slack.com/)
#### [NRF51 SoC](https://www.nordicsemi.com/Products/Low-power-short-range-wireless/nRF51822)






