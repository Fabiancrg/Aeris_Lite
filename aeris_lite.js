import * as m from 'zigbee-herdsman-converters/lib/modernExtend';

export default {
    zigbeeModel: ['aeris-z'],
    model: 'aeris-z',
    vendor: 'ESPRESSIF',
    description: 'Automatically generated definition',
    extend: [m.deviceEndpoints
        (
            {
                endpoints:{"1":1,"2":2,"3":3,"4":4,"5":5,"6":6,"7":7}}), 
                m.temperature(
                    {
                        endpointNames: ["1"],
                        unit: "°C",
                        access: "STATE_GET",
                        precision: 2,
                        reporting: {min: 10, max: 3600, change: 0.1},
                    }
                ), 
                m.humidity(
                    {
                        endpointNames: ["1"],
                        unit: "%",
                        access: "STATE_GET",
                        precision: 1,
                        reporting: {min: 10, max: 3600, change: 1},
                    }
                ),
                m.numeric(
                    {
                        name: "temperature_offset",
                        valueMin: -100,
                        valueMax: 100,
                        valueStep: 1,
                        unit: "0.1°C",
                        cluster: "msTemperatureMeasurement",
                        attribute: {ID: 0xF00F, type: 0x29},  // INT16S
                        description: "Temperature calibration offset (in 0.1°C, e.g., 30 = subtract 3.0°C)",
                        access: "ALL",
                        endpointNames: ["1"]
                    }
                ),
                m.numeric(
                    {
                        name: "humidity_offset",
                        valueMin: -100,
                        valueMax: 100,
                        valueStep: 1,
                        unit: "0.1%",
                        cluster: "msTemperatureMeasurement",
                        attribute: {ID: 0xF010, type: 0x29},  // INT16S
                        description: "Humidity calibration offset (in 0.1%, e.g., 10 = subtract 1.0%)",
                        access: "ALL",
                        endpointNames: ["1"]
                    }
                ),
                m.numeric(
                    {
                        name: "sensor_refresh_interval",
                        valueMin: 10,
                        valueMax: 3600,
                        valueStep: 1,
                        unit: "s",
                        cluster: "msTemperatureMeasurement",
                        attribute: {ID: 0xF011, type: 0x21},  // UINT16
                        description: "Sensor refresh interval in seconds (10-3600, default 30)",
                        access: "ALL",
                        endpointNames: ["1"]
                    }
                ),
                m.pressure(
                    {
                        endpointNames: ["2"],
                        unit: "hPa",
                        access: "STATE_GET",
                        precision: 1,
                        reporting: {min: 10, max: 3600, change: 1},
                    }
                ), 
                m.numeric(
                    {
                        endpointNames:["3"],
                        name:"voc_index",
                        cluster:"genAnalogInput",
                        attribute:"presentValue",
                        reporting:{"min":"MIN","max":"MAX","change":1},
                        description:"VOC Index (1-500, 100=normal)",
                        access:"STATE_GET"
                    }
                ), 
                m.numeric(
                    {
                        endpointNames:["4"],
                        name:"nox_index",
                        cluster:"genAnalogInput",
                        attribute:"presentValue",
                        reporting:{"min":"MIN","max":"MAX","change":1},
                        description:"NOx Index (1-500, 1=normal)",
                        access:"STATE_GET"

                    }
                ), 
                m.co2(
                    {
                        endpointNames:["5"],
                        unit: "ppm",
                        scale: 1,  
                        access: "STATE_GET",
                    }
                ), 
                m.onOff(
                    {
                        powerOnBehavior:false,                  
                        endpointNames:["6"],
                        description: "Master enable for all sensor indicator LEDs"
                    }
                ), 
                m.onOff(
                    {
                        powerOnBehavior:false,                  
                        endpointNames:["7"],
                        description: "Status LED (network connection indicator)"
                    }
                ), 
                m.numeric(
                    {
                        name:"led_mask",
                        valueMin:0,
                        valueMax:15,
                        valueStep:1,
                        cluster:"genAnalogOutput",
                        attribute:"presentValue",
                        description:"LED enable bitmask: bit0=CO2, bit1=VOC, bit2=NOx, bit3=Humidity (15=all on)",
                        access:"ALL",
                        endpointNames:["6"]
                    }
                ),
                m.numeric(
                    {
                        name:"led_brightness",
                        valueMin:1,
                        valueMax:255,
                        valueStep:1,
                        unit:"",
                        cluster:"genLevelCtrl",
                        attribute:"currentLevel",
                        description:"LED brightness level (1=min, 32=default, 255=max)",
                        access:"ALL",
                        endpointNames:["6"]
                    }
                )
            ],
};
