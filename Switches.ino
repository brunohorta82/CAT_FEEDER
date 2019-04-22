#define RELAY_TYPE "relay"
#define MQTT_TYPE "mqtt"
#define SWITCH_DEVICE "switch"
#define BUTTON_SWITCH 1
#define BUTTON_PUSH 2
#define AUTO_OFF 6
#define BUTTON_SET_PULLUP true
#define INIT_STATE_OFF false

#define DEFAULT_DELAY_CAT 1000l

JsonArray& sws = getJsonArray();

typedef struct {
    long delayOff;
    long onTime;
    int gpio;
    Bounce* debouncer;
    String id;
    bool pullup;
    int mode;
    bool state;
    bool locked;
    
} switch_t;
std::vector<switch_t> _switchs;

const String switchsFilename = "switchs.json";

JsonArray& saveSwitch(JsonArray& _switchs){
    for (unsigned int s=0; s < _switchs.size();s++) {
    JsonObject& _switch = _switchs.get<JsonVariant>(s);
      int switchFound = false;
   String type = _switch.get<String>("type");
   String _id = _switch.get<String>("id");
  for (unsigned int i=0; i < sws.size(); i++) {
     JsonObject& switchJson = sws.get<JsonVariant>(i);
    if(switchJson.get<String>("id").equals(_id)){
      switchFound = true;
      removeComponentHaConfig(getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix"),getConfigJson().get<String>("nodeId"),switchJson.get<String>("type"),switchJson.get<String>("class"),switchJson.get<String>("id"));
      String _name = _switch.get<String>("name");
      switchJson.set("gpio",_switch.get<unsigned int>("gpio"));
      switchJson.set("name",_name);
      switchJson.set("delayOff",max(DEFAULT_DELAY_CAT,_switch.get<long>("delayOff")));
      switchJson.set("spot",_switch.get<String>("spot"));
      switchJson.set("discoveryDisabled",_switch.get<bool>("discoveryDisabled"));
      switchJson.set("pullup",_switch.get<bool>("pullup"));
      int swMode = _switch.get<unsigned int>("mode");
      switchJson.set("mode",swMode);
      String typeControl = _switch.get<String>("typeControl");
      switchJson.set("typeControl",typeControl);
      switchJson.set("pullState",0);
      switchJson.set("type",_switch.get<String>("type"));     
      
        switchJson.set("gpioControl",_switch.get<unsigned int>("gpioControl"));
       
       
      String mqttCommand = MQTT_COMMAND_TOPIC_BUILDER(_id,SWITCH_DEVICE,_name);
      switchJson.set("mqttCommandTopic",mqttCommand);
      switchJson.set("mqttStateTopic",MQTT_STATE_TOPIC_BUILDER(_id,SWITCH_DEVICE,_name));
      subscribeOnMqtt(mqttCommand);
    }
  }
  if(!switchFound){
      String _name = _switch.get<String>("name");
      String _id = "B"+String(millis());
      String typeControl = _switch.get<String>("typeControl");
      switchJson(_id,_switch.get<unsigned int>("gpio"),_switch.get<unsigned int>("gpioOpenClose"),_switch.get<unsigned int>("gpio"),typeControl,_switch.get<unsigned int>("gpioControl"),_switch.get<unsigned int>("gpioControlOpen"),_switch.get<unsigned int>("gpioControlClose"),INIT_STATE_OFF,_name, _switch.get<bool>("pullup"),INIT_STATE_OFF,  _switch.get<unsigned int>("mode"), MQTT_STATE_TOPIC_BUILDER(_id,SWITCH_DEVICE,_name), MQTT_COMMAND_TOPIC_BUILDER(_id,SWITCH_DEVICE,_name), _switch.get<String>("type"),_switch.get<long>("delayOff") );
  }
  }
  saveSwitchs();
  applyJsonSwitchs();
 reloadDiscovery();
  return sws;
 }
 
 
void applyJsonSwitchs(){
  _switchs.clear();
  for(int i  = 0 ; i < sws.size() ; i++){ 
    JsonObject& switchJson = sws.get<JsonVariant>(i);   
    int gpio= switchJson.get<unsigned int>("gpio");
    bool lock=switchJson.get<bool>("childLockStateControl");
    Serial.println(lock);        
    bool pullup =switchJson.get<bool>("pullup");
    bool state =switchJson.get<bool>("state");
    int gpioControl = switchJson.get<unsigned int>("gpioControl");
    if ( gpio == 16) {
      configGpio(gpio, INPUT_PULLDOWN_16);
    } else {
      configGpio(gpio, pullup ? INPUT_PULLUP  : INPUT);
    }
    
    Bounce* debouncer = new Bounce();
    if( gpio != 99){
      debouncer->attach(gpio);
      debouncer->interval(5);
    }// interval in ms
    _switchs.push_back({switchJson.get<long>("delayOff"),0,gpio,debouncer,switchJson.get<String>("id"),pullup,switchJson.get<unsigned int>("mode"),state,lock});
    if(switchJson.get<unsigned int>("mode") == AUTO_OFF){
      switchJson.set("stateControl",false);
    }
    initNormal(switchJson.get<bool>("stateControl"),gpioControl);
  }
}

void toogleSwitch(String id) {
  for (unsigned int i=0; i < _switchs.size(); i++) {
    if(  _switchs[i].id.equals(id)){
      _switchs[i].onTime = millis();
      Serial.println(_switchs[i].onTime);
    }
    
   }
  for (unsigned int i=0; i < sws.size(); i++) {
    JsonObject& switchJson = sws.get<JsonVariant>(i);
    if(switchJson.get<String>("id").equals(id)){
      if(switchJson.get<String>("typeControl").equals(RELAY_TYPE)){
        if(switchJson.get<unsigned int>("mode") == AUTO_OFF){
          switchJson.set("stateControl",turnOnRelayNormal( switchJson.get<unsigned int>("gpioControl")));
        } 
      }
    }
  }   
}

void mqttSwitchControl(String topic, String payload) {
 
  for (unsigned int i=0; i < sws.size(); i++) {
    JsonObject& switchJson = sws.get<JsonVariant>(i);
    if(switchJson.get<String>("mqttCommandTopic").equals(topic)){
    if(switchJson.get<String>("typeControl").equals(RELAY_TYPE)){
       for (unsigned int i=0; i < _switchs.size(); i++) {
    if(  _switchs[i].id.equals(switchJson.get<String>("id"))){
      _switchs[i].onTime = millis();
      }
      }
       int gpio = switchJson.get<unsigned int>("gpioControl");
      if(String(PAYLOAD_ON).equals(payload)){
        turnOn(getRelay(gpio));
        }else if (String(PAYLOAD_OFF).equals(payload)){
        turnOff( getRelay(gpio));  
       }
    }else  if(switchJson.get<String>("typeControl").equals(MQTT_TYPE)){
      toogleSwitch(switchJson.get<String>("id"));
    }
    }
   }
 }   


void triggerSwitch(bool _state,  String id, int gpio) {
   for (unsigned int i=0; i < sws.size(); i++) {
    JsonObject& switchJson = sws.get<JsonVariant>(i);
    if(switchJson.get<String>("id").equals(id)){
      switchJson.set("state",_state);
      if(switchJson.get<String>("typeControl").equals(RELAY_TYPE)){
       
        
           if( switchJson.get<unsigned int>("mode") == AUTO_OFF){
              if(_state){
                  turnOn( getRelay(switchJson.get<unsigned int>("gpioControl")));
                }else{
                  turnOff( getRelay(switchJson.get<unsigned int>("gpioControl")));
                }
            }else{
              bool gpioState = toogleNormal(switchJson.get<unsigned int>("gpioControl"));
               switchJson.set("stateControl",gpioState);  
            }
         
    
      }else if(switchJson.get<String>("typeControl").equals(MQTT_TYPE)){
         toogleSwitch(switchJson.get<String>("id"));
      }
    }
   }   
}

void publishState(JsonObject& switchJson){
    saveSwitchs();
    String swtr = "";
    switchJson.printTo(swtr);
    publishOnEventSource("switch",swtr);
    publishOnMqtt(switchJson.get<String>("mqttStateTopic").c_str(),switchJson.get<bool>("stateControl") ? PAYLOAD_ON : PAYLOAD_OFF,true);   
}

void switchNotify(int gpio, bool _gpioState){
  for (unsigned int i=0; i < sws.size(); i++) {
     JsonObject& switchJson = sws.get<JsonVariant>(i);
    if(switchJson.get<unsigned int>("gpioControl") == gpio){
      switchJson.set("stateControl",_gpioState);
      publishState( switchJson);
    }
  }
  
}

JsonArray& getStoredSwitchs(){
  return sws;
}

void loadStoredSwitchs(){
  bool loadDefaults = false;
  if(SPIFFS.begin()){
    File cFile;   

    if(SPIFFS.exists(switchsFilename)){
      cFile = SPIFFS.open(switchsFilename,"r+"); 
      if(!cFile){
        logger("[SWITCH] Create file switchs Error!");
        return;
      }
        logger("[SWITCH] Read stored file config...");
       JsonArray& storedSwitchs = getJsonArray(cFile);       
      for(int i = 0 ; i< storedSwitchs.size(); i++){
        sws.add(storedSwitchs.get<JsonVariant>(i));
        }
        if (!storedSwitchs.success()) {
         logger("[SWITCH] Json file parse Error!");
          loadDefaults = true;
        }else{
          logger("[SWITCH] Apply stored file config...");
          applyJsonSwitchs();
        }
        
     }else{
        loadDefaults = true;
     }
    cFile.close();
     if(loadDefaults){
      logger("[SWITCH] Apply default config...");
      cFile = SPIFFS.open(switchsFilename,"w+");
      createDefaultSwitchs();
      sws.printTo(cFile);
      applyJsonSwitchs();
      cFile.close();
      }
     
  }else{
     logger("[SWITCH] Open file system Error!");
  }
   SPIFFS.end(); 
   
}

void saveSwitchs(){
   if(SPIFFS.begin()){
      logger("[SWITCH] Open "+switchsFilename);
      File rFile = SPIFFS.open(switchsFilename,"w+");
      if(!rFile){
        logger("[SWITCH] Open switch file Error!");
      } else {
       
      sws.printTo(rFile);
      }
      rFile.close();
   }else{
     logger("[SWITCH] Open file system Error!");
  }
  SPIFFS.end();
  logger("[SWITCH] New switch config loaded.");
}

void switchJson(String _id,int _gpio ,int _gpioOpen ,int _gpioClose ,String _typeControl, int _gpioControl,int _gpioControlOpen,int _gpioControlClose, bool _stateControl,  String _name, bool _pullup, bool _state, int _mode,  String _mqttStateTopic, String _mqttCommandTopic, String _type, long delayOff){
    JsonObject& switchJson =  getJsonObject();
      switchJson.set("id", _id);
      switchJson.set("gpio", _gpio);
      switchJson.set("pullup", _pullup);
      if(_typeControl.equals(RELAY_TYPE)){
        switchJson.set("gpioControl", _gpioControl);
      }
      switchJson.set("typeControl", _typeControl);
      switchJson.set("stateControl", _stateControl);
      switchJson.set("mqttStateTopic", _mqttStateTopic);
      switchJson.set("mqttCommandTopic", _mqttCommandTopic);
      switchJson.set("mqttRetain", true);
      switchJson.set("name", _name);
      switchJson.set("mode", _mode);
      switchJson.set("state", _state);
      switchJson.set("childLockStateControl", false);
      switchJson.set("type", _type);
      switchJson.set("class", SWITCH_DEVICE);
      switchJson.set("delayOff",max(DEFAULT_DELAY_CAT,delayOff));
      sws.add(switchJson);
}
void rebuildSwitchMqttTopics( String oldPrefix,String oldNodeId){
      bool store = false;
      JsonArray& _devices = getStoredSwitchs();
      for(int i  = 0 ; i < _devices.size() ; i++){ 
        store = true;
      JsonObject& switchJson = _devices[i];
      removeComponentHaConfig(oldPrefix,oldNodeId,switchJson.get<String>("type"),switchJson.get<String>("class"),switchJson.get<String>("id"));      
      String id = switchJson.get<String>("id");
      String name = switchJson.get<String>("name");
      switchJson.set("mqttCommandTopic",MQTT_COMMAND_TOPIC_BUILDER(id,SWITCH_DEVICE,name));
      switchJson.set("mqttStateTopic",MQTT_STATE_TOPIC_BUILDER(id,SWITCH_DEVICE,name));
      subscribeOnMqtt(switchJson.get<String>("mqttCommandTopic"));
    }
    if(store){
      saveSwitchs();
      reloadDiscovery(); 
      
    }
  }


void createDefaultSwitchs(){
    String id1 = "B1";
      switchJson(id1,99,0,0,RELAY_TYPE,RELAY_MOTOR,0,0,INIT_STATE_OFF,  "Cat Feeder", BUTTON_SET_PULLUP,INIT_STATE_OFF,  AUTO_OFF, MQTT_STATE_TOPIC_BUILDER(id1,SWITCH_DEVICE,"cat_feeder"), MQTT_COMMAND_TOPIC_BUILDER(id1,SWITCH_DEVICE,"cat_feeder"), "switch",DEFAULT_DELAY_CAT);  
}
void removeSwitch(String _id){
  int switchFound = false;
  int index = 0;
  for (unsigned int i=0; i < sws.size(); i++) {
    JsonObject& switchJson = sws.get<JsonVariant>(i);   
    if(switchJson.get<String>("id").equals(_id)){
      switchFound = true;
      index  = i;
      removeComponentHaConfig(getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix"),getConfigJson().get<String>("nodeId"),switchJson.get<String>("type"),switchJson.get<String>("class"),switchJson.get<String>("id"));
    }
  }
  if(switchFound){
    sws.remove(index);
     
    }

  saveSwitchs();
  applyJsonSwitchs();
  
  reloadDiscovery(); 
 
}
void loopSwitchs(){
    for (unsigned int i=0; i < _switchs.size(); i++) {
      if(_switchs[i].locked){
        continue;
        }
       if(_switchs[i].gpio != 99){
        Bounce* b =   _switchs[i].debouncer;
        b->update();
        bool value =  b->read();
        value = _switchs[i].pullup ? !value : value;
      }
      
      int swmode = _switchs[i].mode;
       if(swmode == AUTO_OFF) {
        if(_switchs[i].onTime > 0 && _switchs[i].onTime + _switchs[i].delayOff < millis()){
          _switchs[i].onTime = 0;
          triggerSwitch( false, _switchs[i].id, _switchs[i].gpio);
          continue;
          }
        }
    }
}
 
