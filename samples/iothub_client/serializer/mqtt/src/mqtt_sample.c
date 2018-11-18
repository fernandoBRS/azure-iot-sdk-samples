#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"
#include "serializer.h"
#include "iothub_client_ll.h"
#include "iothubtransportmqtt.h"

static const char* connectionString = "HostName=sample-hub.azure-devices.net;DeviceId=weather-station-001;SharedAccessKey=OsMWUbru1aUu4m3mPH7yLjUq97dfqXeKnfpAjOeNjwY=";
static const IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol = MQTT_Protocol;
static char propText[1024];

/* 
IoT Hub refers to the data a device sends to it as events (WITH_DATA).
IoT Hub refers to the data it sends to devices as messages (WITH_ACTION)
*/

BEGIN_NAMESPACE(WeatherStation);

DECLARE_MODEL(ContosoAnemometer, 
    WITH_DATA(ascii_char_ptr, DeviceId),
    WITH_DATA(int, WindSpeed),
    WITH_DATA(float, Temperature),
    WITH_DATA(float, Humidity),
    WITH_ACTION(TurnFanOn),
    WITH_ACTION(TurnFanOff),
    WITH_ACTION(SetAirResistance, int, Position)
);

END_NAMESPACE(WeatherStation);

EXECUTE_COMMAND_RESULT TurnFanOn(ContosoAnemometer* device) {
    (void) device;
    (void) printf("Turning fan on.\r\n");

    return EXECUTE_COMMAND_SUCCESS; 
}

EXECUTE_COMMAND_RESULT TurnFanOff(ContosoAnemometer* device) {
    (void) device;
    (void) printf("Turning fan off.\r\n");

    return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT SetAirResistance(ContosoAnemometer* device, int Position) {
    (void) device;
    (void) printf("Setting Air Resistance Position to %d.\r\n", Position);
    
    return EXECUTE_COMMAND_SUCCESS;
}

void send_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT confirmation_result, void* userContextCallback) {
    unsigned int tracking_id = (unsigned int)(uintptr_t) userContextCallback;
    const char *result = ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, confirmation_result);

    (void) printf("Message Id: %u Received.\r\n", tracking_id);
    (void) printf("Result callback called: %s \r\n", result);
}

static void send_message(IOTHUB_CLIENT_LL_HANDLE client, const unsigned char* buffer, size_t size, ContosoAnemometer *myWeather) {
    static unsigned int tracking_id;

    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, size);

    if (messageHandle == NULL) {
        printf("Unable to create a new IoTHubMessage\r\n");
    } else {
        MAP_HANDLE propMap = IoTHubMessage_Properties(messageHandle);

        (void) sprintf_s(propText, sizeof(propText), myWeather->Temperature > 28 ? "true" : "false");

        if (Map_AddOrUpdate(propMap, "temperatureAlert", propText) != MAP_OK) {
            (void) printf("ERROR: Map_AddOrUpdate failed!\r\n");
        }

        IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendEventAsync(client, messageHandle, send_callback, (void*)(uintptr_t) tracking_id);
        
        char *response = result == IOTHUB_CLIENT_OK 
            ? "Message sent!\r\n"
            : "Failed to hand over the message to IoT Hub";

        printf("%s\n", response);
        
        IoTHubMessage_Destroy(messageHandle);
    }

    tracking_id++;
}

/* This function "links" IoTHub to the serialization library*/
static IOTHUBMESSAGE_DISPOSITION_RESULT receive_message_callback(IOTHUB_MESSAGE_HANDLE message, void* context_callback) {
    const unsigned char* buffer;
    size_t message_size;

    if (IoTHubMessage_GetByteArray(message, &buffer, &message_size) != IOTHUB_MESSAGE_OK) {
        (void) printf("Unable to retrieve message bytes.\r\n");
        return IOTHUBMESSAGE_ACCEPTED;
    }

    /*buffer is not zero terminated*/
    char* temp = malloc(message_size + 1);

    if (temp == NULL) {
        (void) printf("failed to malloc\r\n");
        return IOTHUBMESSAGE_ACCEPTED;
    }

    (void) memcpy(temp, buffer, message_size);
    
    temp[message_size] = '\0';
    EXECUTE_COMMAND_RESULT command_result = EXECUTE_COMMAND(context_callback, temp);

    if (command_result != EXECUTE_COMMAND_SUCCESS) {
        (void) printf("Execute command failed.\r\n");
    }

    free(temp);

    // MQTT can only accept messages
    return IOTHUBMESSAGE_ACCEPTED;
}

bool valid_init() {
    if (platform_init() != 0) {
        (void) printf("Failed to initialize platform.\r\n");
        return false;
    }

    if (serializer_init(NULL) != SERIALIZER_OK) {
        (void) printf("Failed to initialize the serializer.\r\n");
        platform_deinit();
        return false;
    }

    return true;
}

IOTHUB_CLIENT_LL_HANDLE create_iothub_client() {
    return IoTHubClient_LL_CreateFromConnectionString(connectionString, protocol);
}

void deinitialize() {
    serializer_deinit();
    platform_deinit();
}

void destroy(IOTHUB_CLIENT_LL_HANDLE iothub_client, ContosoAnemometer* weather) {
    DESTROY_MODEL_INSTANCE(weather);
    IoTHubClient_LL_Destroy(iothub_client);
}

bool valid_message_callback(IOTHUB_CLIENT_LL_HANDLE iothub_client, ContosoAnemometer* weather) {
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SetMessageCallback(iothub_client, receive_message_callback, weather);
    return result == IOTHUB_CLIENT_OK;
}

void mqtt_sample_run(void) {
    if (!valid_init()) {
        return;
    }

    IOTHUB_CLIENT_LL_HANDLE iothub_client = create_iothub_client();
    
    if (iothub_client == NULL) {
        (void) printf("Failed to create the IoT Hub client, verify the connection string.\r\n");

        deinitialize();
        return;
    }
    
    ContosoAnemometer* weather = CREATE_MODEL_INSTANCE(WeatherStation, ContosoAnemometer);

    if (weather == NULL) {
        (void) printf("Failed on CREATE_MODEL_INSTANCE\r\n");

        IoTHubClient_LL_Destroy(iothub_client);
        deinitialize();
        return;
    }
    
    if (!valid_message_callback(iothub_client, weather)) {
        printf("Unable to create the message callback.\r\n");

        destroy(iothub_client, weather);
        deinitialize();

        return;
    }
    
    int avgWindSpeed = 10;
    float minTemperature = 20.0;
    float minHumidity = 60.0;

    weather->DeviceId = "weather-station-001";
    weather->WindSpeed = avgWindSpeed + (rand() % 4 + 2);
    weather->Temperature = minTemperature + (rand() % 10);
    weather->Humidity = minHumidity + (rand() % 20);
    {
        unsigned char* destination;
        size_t destinationSize;

        CODEFIRST_RESULT result = SERIALIZE(&destination, &destinationSize, 
            weather->DeviceId, weather->WindSpeed, weather->Temperature, weather->Humidity);
        
        if (result != CODEFIRST_OK) {
            (void) printf("Failed to serialize\r\n");
        }
        else {
            send_message(iothub_client, destination, destinationSize, weather);
            free(destination);
        }
    }

    /* wait for commands */
    while (1) {
        IoTHubClient_LL_DoWork(iothub_client);
        ThreadAPI_Sleep(100);
    }

    destroy(iothub_client, weather);
    deinitialize();
}
