import asyncio
import paho.mqtt.client as mqtt
from fastapi import FastAPI, WebSocket
from fastapi.responses import HTMLResponse
from threading import Thread
import json
import os
import pandas as pd
import joblib
import platform
from datetime import datetime, timedelta, timezone 
from fastapi.staticfiles import StaticFiles

app = FastAPI()
app.mount("/static", StaticFiles(directory="static"), name="static")

mqtt_queue = asyncio.Queue()

system_config = {
    "is_monitoring": False
}
connected_devices = {}
device_params = {
    "features": None,
    "state": None,
    "timestamp": None
}


class AnalyseSensorData():
    
    models      = None 
    names       = None
    predictions = None
    
    def __init__(self, sensor_data):
        
        self.names = list()
        self.models = dict()
        self.predictions = dict()
        self.dst = self.process_dataset(sensor_data)

        self.load_models()
        self.make_prediction()

    def get_source_directory(self):
        root = os.getcwd()
        return os.path.join(root, "models")

    def process_dataset(self, sensor_data):
        json_data = list()
        json_data.append(sensor_data)
        df = pd.DataFrame(json_data)
        X = df.to_numpy()
        return X
    
    def retrieve_name(self, file_name):
        if  platform.system() == 'Windows':
            file_name = file_name.split("\\")[-1]
        else: 
            file_name = file_name.split("/")[-1]

        period_index = file_name.rfind(".")
        return file_name[:period_index]

    def load_models(self):
        model_directory = self.get_source_directory()
        try:
            model_list = os.listdir(model_directory)
            if len(model_list) > 0:
                for file in model_list:
                    name = self.retrieve_name(file)
                    self.models[name] = joblib.load(os.path.join(model_directory, file))
                    self.names.append(name)
            else:
                raise ValueError('No saved model found in "{}" directory.'.format(source_dir))
        except Exception as e:
            print("FATAL MODEL LOAD ERROR :  ", dir(e))
            exit()
            
    def make_prediction(self):
        try:
            for name in self.names:
                self.predictions[name] = self.models[name].predict(self.dst)
        except Exception as e:
            print("FATAL PREDICTION ERROR:  ", dir(e))
            exit()

    @staticmethod
    def make_predictions(sensor_data):
        analysis = AnalyseSensorData(sensor_data)
        num_of_models = len(list(analysis.predictions.keys()))
        sum_of_predictions = sum(analysis.predictions.values())
        if num_of_models > 0:
            prediction = sum_of_predictions / num_of_models
            return 1 if prediction >= 0.5 else 0
        else:
            raise ValueError("FATAL ERROR: NO TRAINED ML MODEL FOUND!!!")
            exit()

# HTML content for the web UI
HTML_PAGE = """
<!DOCTYPE html>
<html>
<head>
    <title>Fire Emergency Response System (FERS)</title>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" integrity="sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH" crossorigin="anonymous">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.7.1/css/all.min.css" integrity="sha512-5Hs3dF2AEPkpNAR7UiOHba+lRSJNeM2ECkwxUIxC1Q/FLycGTbNapWXB4tP889k5T5Ju8fs4b1P5z/iB4nMfSQ==" crossorigin="anonymous" referrerpolicy="no-referrer" />
    <link rel="stylesheet" href="/static/app.css">
    <script defer src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.min.js" integrity="sha384-0pUGZvbkm6XF6gxjEnlmuGrJXVbNuzT9qBBavbLwCsOGabYfZo0T0to5eqruptLy" crossorigin="anonymous"></script>
    <script src="/static/app.js" defer></script>
</head>
<body>
    <div class="container">
        <div class="row">
            <div class="col-12 ">
                <h1 class="application-title">Fire Emergency Response System (FERS) <br /> Monitor Application</h1>
                <div class="monitor-toggle">
                    <label class="switch">
                        <input type="checkbox" id="monitor-toggle">
                        <span class="slider" id="monitor-toggle-span"></span>
                    </label>
                    <span>Monitor</span>
                </div>
                <div class="watch" id="system-watch"></div>
            </div>
        </div>
        <div class="row">
            <div class="col-12">
                <div class="row" id="output">
                    <div class="p-5 text-center bg-body-tertiary rounded-3">
                        <h1 class="text-body-emphasis">FERS TURNED OFF!</h1>
                        <p class="col-lg-8 mx-auto fs-5 text-muted">
                            Turn on the Moniter to run FERS System.
                        </p>
                    </div>
                </div>
            </div>
        </div>
    </div>
</body>
</html>
"""

@app.get("/")
async def get():
    return HTMLResponse(content=HTML_PAGE)

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    while True:
        message = await mqtt_queue.get()
        await websocket.send_json(message)

# MQTT client configuration
def on_connect(client, userdata, flags, rc, properties):
    print(f"Connected with result code {rc}")
    client.subscribe("Remote/FERS/IoT-Data/json")

def on_message(client, userdata, msg):
    # Parse the incoming MQTT message payload
    json_data = json.loads(msg.payload)

    node = json_data["Node"]
    node_timestamp = json_data["Timestamp"]
    current_timestamp = datetime.now()

    # Clean up stale devices from connected_devices
    stale_nodes = [
        key for key, device_info in connected_devices.items()
        if abs(current_timestamp - datetime.fromisoformat(device_info["timestamp"])) > timedelta(minutes=120)
    ]
    
    for stale_node in stale_nodes:
        del connected_devices[stale_node]

    # Ensure the node exists in connected_devices, initialize if new
    if node not in connected_devices:
        connected_devices[node] = {
            "features": {},
            "state": None,
            "timestamp": None
        }

    # Remove unnecessary keys from json_data
    json_data.pop("Timestamp", None)
    json_data.pop("Node", None)

    # Analyze sensor data and update node information
    predicted = AnalyseSensorData.make_predictions(json_data)
    connected_devices[node]["features"].update(json_data)  # Merge new features without overwriting
    connected_devices[node]["state"] = predicted
    connected_devices[node]["timestamp"] = node_timestamp

    asyncio.run(mqtt_queue.put(connected_devices))

def mqtt_worker():
    mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.connect("broker.emqx.io", 1883, 60)
    mqtt_client.loop_forever()

# Start MQTT client in a separate thread
mqtt_thread = Thread(target=mqtt_worker)
mqtt_thread.start()

# Run FastAPI server
if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
