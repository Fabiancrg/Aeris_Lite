import * as m from 'zigbee-herdsman-converters/lib/modernExtend';

export default {
    zigbeeModel: ['aeris-z'],
    model: 'aeris-z',
    vendor: 'ESPRESSIF',
    description: 'Automatically generated definition',
    extend: [m.deviceEndpoints
        (
            {
                endpoints:{"1":1,"2":2,"3":3,"4":4,"5":5,"6":6,"7":7,"8":8,"9":9,"10":10}}), 
                m.temperature(
                    {
                        endpointNames: ["1"],
                        unit: "°C",
                        access: "STATE_GET",
                        precision: 1,
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
                        attribute: 0xF00F,
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
                        attribute: 0xF010,
                        description: "Humidity calibration offset (in 0.1%, e.g., 10 = subtract 1.0%)",
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
                        name:"PM1.0 µg/m3",
                        cluster:"genAnalogInput",
                        attribute:"presentValue",
                        reporting:{"min":"MIN","max":"MAX","change":1},
                        description:"analog_input_3",
                        access:"STATE_GET",
                        endpointNames:["3"]
                    }
                ), 
                m.numeric(
                    {
                        name:"PM2.5 µg/m3",
                        cluster:"genAnalogInput",
                        attribute:"presentValue",
                        reporting:{"min":"MIN","max":"MAX","change":1},
                        description:"analog_input_4",
                        access:"STATE_GET",
                        endpointNames:["4"]
                    }
                ), 
                m.numeric(
                    {
                        name:"PM10 µg/m3",
                        cluster:"genAnalogInput",
                        attribute:"presentValue",
                        reporting:{"min":"MIN","max":"MAX","change":1},
                        description:"analog_input_5",
                        access:"STATE_GET",
                        endpointNames:["5"]
                    }
                ), 
                m.numeric(
                    {
                        endpointNames:["6"],
                        name:"VOC Index",
                        cluster:"genAnalogInput",
                        attribute:"presentValue",
                        reporting:{"min":"MIN","max":"MAX","change":1},
                        description:"analog_input_6",
                        access:"STATE_GET"
                    }
                ), 
                m.numeric(
                    {
                        endpointNames:["7"],
                        name:"NOx Index",
                        cluster:"genAnalogInput",
                        attribute:"presentValue",
                        reporting:{"min":"MIN","max":"MAX","change":1},
                        description:"analog_input_7",
                        access:"STATE_GET"

                    }
                ), 
                m.co2(
                    {
                        endpointNames:["8"],
                        unit: "ppm",
                        scale: 1,  
                        access: "STATE_GET",
                    }
                ), 
                m.onOff(
                    {
                        powerOnBehavior:false,                  
                        endpointNames:["9"],
                        description: "Master enable for all sensor indicator LEDs"
                    }
                ), 
                m.onOff(
                    {
                        powerOnBehavior:false,                  
                        endpointNames:["10"],
                        description: "Status LED (network connection indicator)"
                    }
                ), 
                m.numeric(
                    {
                        name:"led_mask",
                        valueMin:0,
                        valueMax:31,
                        valueStep:1,
                        cluster:"genAnalogOutput",
                        attribute:"presentValue",
                        description:"LED enable bitmask: bit0=CO2, bit1=VOC, bit2=NOx, bit3=PM2.5, bit4=Humidity (31=all on)",
                        access:"ALL",
                        endpointNames:["9"]
                    }
                ),
                m.numeric(
                    {
                        name:"led_brightness",
                        valueMin:0,
                        valueMax:255,
                        valueStep:1,
                        unit:"",
                        cluster:"genLevelCtrl",
                        attribute:"currentLevel",
                        description:"LED brightness level (0=off, 32=default, 255=max)",
                        access:"ALL",
                        endpointNames:["9"]
                    }
                )
            ],
};
