import * as m from 'zigbee-herdsman-converters/lib/modernExtend';

export default {
    zigbeeModel: ['aeris-z'],
    model: 'aeris-z',
    vendor: 'ESPRESSIF',
    description: 'Automatically generated definition',
    extend: [m.deviceEndpoints
        (
            {
                "endpoints":{"1":1,"2":2,"3":3,"4":4,"5":5,"6":6,"7":7,"8":8,"9":9,"10":10}}), 
                m.temperature(), 
                m.humidity(), 
                m.pressure(
                    {
                        "endpointNames":["2"]
                    }
                ), 
                m.numeric(
                    {
                        "name":"PM1.0 µg/m3",
                        "cluster":"genAnalogInput",
                        "attribute":"presentValue",
                        "reporting":{"min":"MIN","max":"MAX","change":1},
                        "description":"analog_input_3",
                        "access":"STATE_GET",
                        "endpointNames":["3"]
                    }
                ), 
                m.numeric(
                    {
                        "name":"PM2.5 µg/m3",
                        "cluster":"genAnalogInput",
                        "attribute":"presentValue",
                        "reporting":{"min":"MIN","max":"MAX","change":1},
                        "description":"analog_input_4",
                        "access":"STATE_GET",
                        "endpointNames":["4"]
                    }
                ), 
                m.numeric(
                    {
                        "name":"PM10 µg/m3",
                        "cluster":"genAnalogInput",
                        "attribute":"presentValue",
                        "reporting":{"min":"MIN","max":"MAX","change":1},
                        "description":"analog_input_5",
                        "access":"STATE_GET",
                        "endpointNames":["5"]
                    }
                ), 
                m.numeric(
                    {
                        "name":"VOC Index",
                        "cluster":"genAnalogInput",
                        "attribute":"presentValue",
                        "reporting":{"min":"MIN","max":"MAX","change":1},
                        "description":"analog_input_6",
                        "access":"STATE_GET",
                        "endpointNames":["6"]
                    }
                ), 
                m.numeric(
                    {
                        "name":"NOx Index",
                        "cluster":"genAnalogInput",
                        "attribute":"presentValue",
                        "reporting":{"min":"MIN","max":"MAX","change":1},
                        "description":"analog_input_7",
                        "access":"STATE_GET",
                        "endpointNames":["7"]
                    }
                ), 
                m.co2(
                    {
                        "endpointNames":["8"]
                    }
                ), 
                m.onOff(
                    {
                        "powerOnBehavior":false,                  
                        "endpointNames":["9","10"]
                    }
                ), 
                m.numeric(
                    {
                        "name":"LED Threshold",
                        "valueMin":0,
                        "valueMax":65535,
                        "valueStep":1,
                        "cluster":"genAnalogOutput",
                        "attribute":"presentValue",
                        "reporting":{"min":"MIN","max":"MAX","change":1},
                        "description":"analog_output_9",
                        "access":"ALL",
                        "endpointNames":["9"]
                    }
                )
            ],
};
