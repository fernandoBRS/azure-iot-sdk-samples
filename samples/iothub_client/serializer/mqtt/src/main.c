/* 
This sample sends and receives messages from IoT Hub through MQTT protocol, using the lower-level APIs of IoT Hub client.
Lower-level APIs of IoT Hub client enables you to send and receive messages without a background thread.

These APIs are useful when the developer wants to have the ability to explicitly control when to send and receive data from IoT Hub.
For example, some devices collect data over time and only ingress events at specified intervals (for example, once an hour or once a day). 
More info: https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-device-sdk-c-iothubclient
*/

#include <stdio.h>
#include "mqtt_sample.h"

int main(void)  
{
    (void) mqtt_sample_run();
    
    return 0;
}
 