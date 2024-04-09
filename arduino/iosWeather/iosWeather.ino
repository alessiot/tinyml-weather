/*
  inoWeather

  Board: Arduino Nano 33 BLE Sense

  by Alessio Tamburro
*/

// bluetooth doc: https://docs.arduino.cc/tutorials/nano-33-ble/bluetooth/
#include <ArduinoBLE.h>
#include <Arduino_HS300x.h>
#include <Arduino_LPS22HB.h> 

// see https://github.com/tensorflow/tflite-micro-arduino-examples/blob/main/examples/hello_world/hello_world.ino (cpp and h for model)
#include "model.h"
// see installation at https://github.com/tensorflow/tflite-micro-arduino-examples 
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/recording_micro_interpreter.h>
#include <tensorflow/lite/micro/micro_profiler.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/system_setup.h>
#include <tensorflow/lite/schema/schema_generated.h>

// rain_model_tflite is the variable holding the data of the TFLite model
extern const unsigned char* g_model = __model_tflite_rain_model_tflite;

#define BLE_UUID_TEMP_VALUE "aaa691a2-3713-46ed-8d85-3a120d760175" // work-aound to write floats through BLE

// Services and characteristics
// - LED
//BLEService switchService("4c40dfe1-7044-4058-8d53-24372c4e2e08");
//BLEByteCharacteristic switchCharacteristic("4c40dfe1-7044-4058-8d53-24372c4e2e08", BLEWrite);// remote device such as phone can write this characteristic
// Service to provide temperature and other characteristics
BLEService weatherService("3c2312f7-7c24-4104-85ad-85a5a039d3cd");
BLEByteCharacteristic weatherCharacteristic( "4c40dfe1-7044-4058-8d53-24372c4e2e08", BLERead | BLENotify );
BLEFloatCharacteristic tempCharacteristic( "aaa691a2-3713-46ed-8d85-3a120d760175", BLERead | BLENotify );
BLEFloatCharacteristic humCharacteristic( "d85e531e-0d16-4e39-902c-417b100e867e", BLERead | BLENotify );


float temperature = 999;// same init as for app
int precipitation = 2;// same init as for app
int humidity = 999;// same init as for app
unsigned long startMillis = 0;
unsigned long currentMillis = 0;

// input scaling variables for model inference
constexpr float t_mean = 13.63919263f;  
constexpr float h_mean = 68.19957846f;
constexpr float p_mean = 941.2085821f;
constexpr float t_std  = 10.13810032f;
constexpr float h_std  = 19.3293953f;
constexpr float p_std  = 147.99274117f;
// This array will store the quantized inputs for inference
constexpr int n_timesteps = 1;
// input array (t,h,p)
float_t inputs_arr[1][n_timesteps][3] = {{{0}}};// T, H, P 
// model related variables
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* model_input = nullptr;
TfLiteTensor* model_output = nullptr;
constexpr int kTensorArenaSize = 4096;
// Keep aligned to 16 bytes for CMSIS
alignas(16) uint8_t tensor_arena[kTensorArenaSize];
// quantization parameters of the input and output tensor
float model_output_scale = 1.0f;
float model_output_zero_point = 0.0f;
float model_input_scale = 1.0f;
float model_input_zero_point = 0.0f;


void setup() {

  // initialize serial communication at 9600 bits per second
  Serial.begin(9600); 
  //while (!Serial); // wait until it starts - this will bock the board if its running on battery because there is no serial there

  // set LED pin to output mode
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);    // LED is turned off
  digitalWrite(LEDR, HIGH);          // will turn the LED off LOW=0, HIGH=255
  digitalWrite(LEDG, HIGH);          
  digitalWrite(LEDB, HIGH);          

  // Sensors initialization
  if (!HS300x.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1);
  }
  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    while (1);
  }

  // BLE initialization
  if (!BLE.begin()) {
    Serial.println("Failed to initialize BLE module!");
    while (1);
  }

  // set advertized local name and service UUIDs
  BLE.setLocalName("iosWeather");
  //BLE.setAdvertisedService(tempService);
  BLE.setAdvertisedService(weatherService);
  //BLE.setAdvertisedService(switchService);
  //BLE.setAdvertisedService(weatherService);
  // add the characteristics to the services
  //switchService.addCharacteristic(switchCharacteristic);
  //tempService.addCharacteristic(tempCharacteristic);
  weatherService.addCharacteristic(weatherCharacteristic);
  weatherService.addCharacteristic(tempCharacteristic);
  weatherService.addCharacteristic(humCharacteristic);
  // add services to BLE stack
  //BLE.addService(switchService);
  //BLE.addService(tempService);
  BLE.addService(weatherService);
  // set the initial value for the characteristic:
  //switchCharacteristic.writeValue(0);  
  // set read request handler for temperature characteristic
  //tempCharacteristic.setEventHandler(BLERead, tempCharacteristicRead);
  // start advertising
  BLE.advertise();

  startMillis = millis();
  Serial.println("Start advertising");

  // TensorFlow init
  tflite::InitializeTarget();
  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    MicroPrintf(
        "Model provided is schema version %d not equal "
        "to supported version %d.",
        model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }
  // This pulls in all the operation implementations we need.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::AllOpsResolver resolver;
  // Build an interpreter to run the model with.
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;
  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return;
  }
  // Obtain pointers to the model's input and output tensors.
  model_input = interpreter->input(0);
  model_output = interpreter->output(0);
  // get the quantization parameters for the input and output tensors
  if (model_input->params.scale==0){
    Serial.println("Model is not quantized");
  }
  else{
    model_input_scale      = model_input->params.scale;
    model_input_zero_point = model_input->params.zero_point;
    model_output_scale      = model_output->params.scale;
    model_output_zero_point = model_output->params.zero_point;
  }

  // Initialize model input data elements to 9999
  for (int i = 0; i < 1; ++i) {
    for (int j = 0; j < n_timesteps; ++j) {
      for (int k = 0; k < 3; ++k) {
        inputs_arr[i][j][k] = 9999;
      }
    }
  }

}

void loop() {  
  // listen for BLE centrals to connect
  BLEDevice central = BLE.central();

  // if a central is connected to peripheral
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    Serial.println("Turning LED on - ready");// orange LED
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.println("Blue LED on - collecting data");
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDG, LOW);
    digitalWrite(LEDB, HIGH);// this is more like purple. To ajust intensity for example 240, etc, use analogWrite

    while (central.connected()) {
      currentMillis = millis();

      // write initial values to display and/or handle in app
      tempCharacteristic.writeValue(temperature);
      weatherCharacteristic.writeValue(precipitation);
      humCharacteristic.writeValue(humidity);

      // read sensor data every X seconds, 1000 == 1sec
      if (currentMillis - startMillis >= 1000) {//use small value for debugging

        float pressure = BARO.readPressure();
        pressure *= 10; //kPa to mb
        temperature = HS300x.readTemperature();
        humidity    = HS300x.readHumidity();
        // print current sensor data
        Serial.print("Temperature = ");
        Serial.print(temperature);
        Serial.println(" °C");
        Serial.print("Humidity = ");
        Serial.print(humidity);
        Serial.println(" %");
        Serial.print("Pressure = ");
        Serial.print(pressure);
        Serial.println(" kPa");
        // update temperature value in temperature characteristic
        tempCharacteristic.writeValue(temperature);

        // scale inputs. 
        float stemperature = (temperature - t_mean) / t_std;
        float shumidity = (humidity - h_mean) / h_std;
        float spressure = (pressure - p_mean) / p_std;
        Serial.print("Scaled Temperature = ");
        Serial.print(stemperature);
        Serial.println(" °C");
        Serial.print("Scaled Humidity = ");
        Serial.print(shumidity);
        Serial.println(" %");
        Serial.print("Scaled Pressure = ");
        Serial.print(spressure);
        Serial.println(" kPa");

        // quantize inputs - this is what the model expects
        // quantize the inputs - what if the model is not quantized?
        /*Serial.print("model_input_scale: ");
        Serial.println(model_input_scale);
        Serial.print("model_input_zero_point: ");
        Serial.println(model_input_zero_point);
        Serial.print("model_output_scale: ");
        Serial.println(model_output_scale);
        Serial.print("model_output_zero_point: ");
        Serial.println(model_output_zero_point);*/
        float qtemperature = (stemperature / model_input_scale) + model_input_zero_point;
        float qhumidity = (shumidity / model_input_scale) + model_input_zero_point;
        float qpressure = (spressure / model_input_scale) + model_input_zero_point;
        /*Serial.print("QTemperature = ");
        Serial.print(temperature);
        Serial.println(" ");
        Serial.print("QHumidity = ");
        Serial.print(humidity);
        Serial.println(" ");
        Serial.print("QPressure = ");
        Serial.print(pressure);
        Serial.println(" ");*/

        /*Serial.println("Current array:");
        for (int i=0; i<n_timesteps; ++i){
          for (int j=0; j<3; ++j){
            Serial.print(inputs_arr[0][i][j]);
            Serial.print(" ");
          }
        }
        Serial.println("");*/

        // find first available index to fill
        int i_first = n_timesteps-1;
        for (int i=0; i<n_timesteps; ++i){
          if (inputs_arr[0][i][0] == 9999){
            i_first = i;
            break;
          }
        }
        //Serial.println(i_first);

        // shift left if last position is filled
        if (i_first == n_timesteps-1){
          // for the first time we fill the last position fill it
          if (inputs_arr[0][i_first][0] == 9999){
            inputs_arr[0][i_first][0] = qtemperature;
            inputs_arr[0][i_first][1] = qhumidity;
            inputs_arr[0][i_first][2] = qpressure;
          }
          else{
            //Serial.println("Shifting to left");
            for (int i = 1; i < n_timesteps; ++i) { 
                for (int j = 0; j < 3; ++j) {
                    inputs_arr[0][i-1][j] = inputs_arr[0][i][j]; // Move the value at index i-1 to index i
                }
            }
          }
        }

        inputs_arr[0][i_first][0] = qtemperature;
        inputs_arr[0][i_first][1] = qhumidity;
        inputs_arr[0][i_first][2] = qpressure;

        /*Serial.println("Modified array:");
        for (int i=0; i<n_timesteps; ++i){
          for (int j=0; j<3; ++j){
            Serial.print(inputs_arr[0][i][j]);
            Serial.print(" ");
          }
          Serial.println(" ");
        }
        Serial.println(" ");*/

        if (i_first==n_timesteps-1){
          // for testing purposes - copy paste code from notebook, this will overwrite values
          /*
          inputs_arr[0][0][0] = 1.6202373504638672;
          inputs_arr[0][0][1] = -0.17638367414474487;
          inputs_arr[0][0][2] = 0.37843433022499084;
          inputs_arr[0][1][0] = 1.6202373504638672;
          inputs_arr[0][1][1] = -0.17638367414474487;
          inputs_arr[0][1][2] = 0.37843433022499084;
          inputs_arr[0][2][0] = 1.6202373504638672;
          inputs_arr[0][2][1] = -0.17638367414474487;
          inputs_arr[0][2][2] = 0.37843433022499084;
          inputs_arr[0][3][0] = 1.6202373504638672;
          inputs_arr[0][3][1] = -0.12958940863609314;
          inputs_arr[0][3][2] = 0.37843433022499084;
          inputs_arr[0][4][0] = 1.6202373504638672;
          inputs_arr[0][4][1] = -0.0827951431274414;
          inputs_arr[0][4][2] = 0.37843433022499084;
          inputs_arr[0][5][0] = 1.6202373504638672;
          inputs_arr[0][5][1] = -0.12958940863609314;
          inputs_arr[0][5][2] = 0.37843433022499084;
          inputs_arr[0][6][0] = 1.6202373504638672;
          inputs_arr[0][6][1] = -0.17638367414474487;
          inputs_arr[0][6][2] = 0.37843433022499084;
          inputs_arr[0][7][0] = 1.6202373504638672;
          inputs_arr[0][7][1] = -0.23084913194179535;
          inputs_arr[0][7][2] = 0.37843433022499084;
          inputs_arr[0][8][0] = 1.6202373504638672;
          inputs_arr[0][8][1] = -0.2853145897388458;
          inputs_arr[0][8][2] = 0.37843433022499084;
          inputs_arr[0][9][0] = 1.6449229717254639;
          inputs_arr[0][9][1] = -0.27329638600349426;
          inputs_arr[0][9][2] = 0.37843433022499084;
          inputs_arr[0][10][0] = 1.6696085929870605;
          inputs_arr[0][10][1] = -0.2612781822681427;
          inputs_arr[0][10][2] = 0.37843433022499084;
          inputs_arr[0][11][0] = 1.6696085929870605;
          inputs_arr[0][11][1] = -0.2612781822681427;
          inputs_arr[0][11][2] = 0.37843433022499084;
          for (int i=0; i<n_timesteps; ++i){
            for (int j=0; j<3; ++j){
              inputsarr[0][i][j] = (inputs_arr[0][i][j] / model_input_scale) + model_input_zero_point;
            }
          }
          */
          //////
          // pointer to input data array
          float_t (*flat_inputs_arr)[n_timesteps][3] = (float_t(*)[n_timesteps][3])model_input->data.f; // for types see https://bytedeco.org/javacpp-presets/tensorflow-lite/apidocs/org/bytedeco/tensorflowlite/TfLitePtrUnion.html
          // assign the pointer to the collected data
          memcpy(flat_inputs_arr, inputs_arr, sizeof(float_t) * 1 * n_timesteps * 3);
          /*Serial.println("Verify input data");
          float_t (*tmp)[n_timesteps][3] = (float_t(*)[n_timesteps][3])model_input->data.f;
          for (int i=0; i<n_timesteps; ++i){
            for (int j=0; j<3; ++j){
              Serial.print(tmp[0][i][j]);
              Serial.print(" ");
            }
            Serial.println(" ");
          }*/
          Serial.println(" ");
          // Run inference, and report any error
          TfLiteStatus invoke_status = interpreter->Invoke();
          if (invoke_status != kTfLiteOk) {
            MicroPrintf("Invoke failed on x: %f\n", 1);
            return;
          }
          Serial.println("TfLiteStatus is OK");

          // Obtain the quantized output from model's output tensor
          float out = model_output->data.f[0];
          Serial.print("QOut = ");
          Serial.println(out);
          // de-quantize
          float out_f = (out - model_output_zero_point) * model_output_scale; 
          Serial.print("Out = ");
          Serial.println(out);
          
          // Get classification labels
          if (out_f > 0.5) {
            Serial.println("Yes, it will rain");
            Serial.println("Red LED on");
            digitalWrite(LEDR, HIGH);
            digitalWrite(LEDG, LOW);
            digitalWrite(LEDB, LOW);
            precipitation = 1;
            weatherCharacteristic.writeValue(precipitation);
          }
          else {
            Serial.println("No, it will not rain");
            Serial.println("Green LED on");
            digitalWrite(LEDR, LOW);
            digitalWrite(LEDG, HIGH);
            digitalWrite(LEDB, LOW);
            precipitation = 0;
            weatherCharacteristic.writeValue(precipitation);
          }
          // print an empty line
          Serial.println();
          Serial.println();
        } // when you reach 12 timesteps, make a prediction

        startMillis = currentMillis;  
      }// every 1 sec

      // if the remote device wrote to the characteristic,
      // use the value to control the LED:
      /*
      if (switchCharacteristic.written()) {
        uint8_t value = switchCharacteristic.value(); // assuming switchCharacteristic.value() returns uint8_t
        switch (value) {
          case 1: // 01, etc. in octal is equivalent to 1 in decimal.
            Serial.println("Red LED on");
            digitalWrite(LEDR, LOW);
            digitalWrite(LEDG, HIGH);
            digitalWrite(LEDB, HIGH);
            break;
          case 2:
            Serial.println("Green LED on");
            digitalWrite(LEDR, HIGH);
            digitalWrite(LEDG, LOW);
            digitalWrite(LEDB, HIGH);
            break;
          case 3:
            Serial.println("Blue LED on");
            digitalWrite(LEDR, HIGH);
            digitalWrite(LEDG, HIGH);
            digitalWrite(LEDB, LOW);
            break;
          default:
            Serial.println(F("LEDs off"));
            digitalWrite(LEDR, HIGH);
            digitalWrite(LEDG, HIGH);
            digitalWrite(LEDB, HIGH);
            break;
        }
      }// write switch 
      */
    }
  }
}

// Read request handler for tempCharacteristic
/*void tempCharacteristicRead(BLEDevice central, BLECharacteristic characteristic) {
  tempCharacteristic.writeValue(temperature);
}*/
