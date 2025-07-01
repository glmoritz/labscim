import json
import base64
import paho.mqtt.client as mqtt
import psycopg2
import time
import struct

MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_USERNAME = ''
MQTT_PASSWORD = ''

# MQTT Topics
UPLINK_TOPIC = "application/+/device/+/event/up"
DOWNLINK_TOPIC_TEMPLATE = "application/{app_id}/device/{dev_eui}/command/down"

SEND_LORA_DOWNSTREAM_REPLY = True


def execute(sql):
    """ Connect to the PostgreSQL database server """
    conn = None
    try:
        # connect to the PostgreSQL server
        print('Connecting to the PostgreSQL database...')        
        conn = psycopg2.connect(
            host="localhost",
            database="chirpstack",
            user="chirpstack",
            password="chirpstack")
		
        # create a cursor
        cur = conn.cursor()
        
	    # execute a statement
        print('PostgreSQL database version:')
        cur.execute('SELECT version()')

        # display the PostgreSQL database server version
        db_version = cur.fetchone()
        print(db_version)

        # execute the UPDATE  statement
        cur.execute(sql)
        
        # Commit the changes to the database
        conn.commit()
        
	    # close the communication with the PostgreSQL
        cur.close()
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)
    finally:
        if conn is not None:
            conn.close()
            print('Database connection closed.')

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe(UPLINK_TOPIC)
    print(f"Subscribed to uplink topic: {UPLINK_TOPIC}")

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())
        app_id = msg.topic.split("/")[1]
        dev_eui = msg.topic.split("/")[3]

        # Extract and decode data
        data_b64 = payload.get("data")
        if not data_b64:
            print("No data found in uplink message.")
            return

        print(f"Received data from {dev_eui}: {data_b64}")



        if SEND_LORA_DOWNSTREAM_REPLY:
            raw = base64.b64decode(data_b64)
            if len(raw) < 8:
                print("Payload too short to contain a uint64.")
                return
            
            value1 = struct.unpack("<Q", raw[:8])[0]
            value2 = int(time.time() * 1_000_000) 

            print(f"Decoded value1: {value1}")
            print(f"Current timestamp (us): {value2}")

            # Pack both values as little-endian uint64
            response_bytes = struct.pack("<QQ", value1, value2)
            response_b64 = base64.b64encode(response_bytes).decode()


            # Prepare downlink payload
            downlink_msg = {
                "dev_eui": dev_eui,
                "confirmed": False,
                "fPort": 2,
                "data": response_b64,  # Echo the same base64 data
            }
            mirror_topic = f"downstreamtopic/{dev_eui}"
            client.publish(mirror_topic, json.dumps(downlink_msg))

            downlink_topic = DOWNLINK_TOPIC_TEMPLATE.format(app_id=app_id, dev_eui=dev_eui)
            client.publish(downlink_topic, json.dumps(downlink_msg))
            print(f"Sent echo to {dev_eui} on topic {downlink_topic}")

    except Exception as e:
        print("Error processing message:", e)

def main():

    #first flush chirpstack nonce
    sql = """ UPDATE device_keys SET dev_nonces = '{"0000000000000000": []}'::jsonb  """    
    execute(sql)

    client = mqtt.Client()
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_forever()


if __name__ == "__main__":
    main()

