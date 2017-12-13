/**
  MQTT433gateway - MQTT 433.92 MHz radio gateway utilizing ESPiLight
  Project home: https://github.com/puuu/MQTT433gateway/

  The MIT License (MIT)

  Copyright (c) 2017 Jan Losinski

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "Settings.h"

#include <ArduinoJson.h>

#include <ArduinoSimpleLogging.h>
#include <StringStream.h>

static inline Settings::SettingTypeSet setFor(const SettingType type) {
  return Settings::SettingTypeSet().set(type);
}

static inline String maskSensible(const String &val, const bool sensible) {
  return (sensible ? val : F("xxx"));
}

template <typename TVal, typename TKey>
bool setIfPresent(JsonObject &obj, TKey key, TVal &var) {
  if (obj.containsKey(key)) {
    TVal tmp = obj.get<TVal>(key);
    if (tmp != var) {
      var = tmp;
      return true;
    }
  }
  return false;
}

void Settings::registerChangeHandler(SettingType setting,
                                     const SettingCallbackFn &callback) {
  listeners.emplace_front(setting, callback);
}

void Settings::onConfigChange(SettingTypeSet typeSet) const {
  for (const auto &listener : listeners) {
    if (typeSet[listener.type]) {
      listener.callback(*this);
    }
  }
}

void Settings::load() {
  // ToDo load

  // Fire for all
  onConfigChange(SettingTypeSet().set());
}

Settings::~Settings() = default;

void Settings::updateProtocols(const String &protocols) {
  this->rfProtocols = protocols;
  onConfigChange(setFor(RF_PROTOCOL));
}

void Settings::updateOtaUrl(const String &otaUrl) { this->otaUrl = otaUrl; }

void Settings::serialize(Print &stream, bool pretty, bool sensible) const {
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();

  root[F("deviceName")] = this->deviceName;
  root[F("mqttReceiveTopic")] = this->mqttReceiveTopic;
  root[F("mqttLogTopic")] = this->mqttLogTopic;
  root[F("mqttRawRopic")] = this->mqttRawRopic;
  root[F("mqttSendTopic")] = this->mqttSendTopic;
  root[F("mqttConfigTopic")] = this->mqttConfigTopic;
  root[F("mqttOtaTopic")] = this->mqttOtaTopic;
  root[F("mqttBroker")] = this->mqttBroker;
  root[F("mqttBrokerPort")] = this->mqttBrokerPort;
  root[F("mqttUser")] = this->mqttUser;
  root[F("mqttPassword")] = maskSensible(this->mqttPassword, sensible);
  root[F("mqttRetain")] = this->mqttRetain;
  root[F("rfReceiverPin")] = this->rfReceiverPin;
  root[F("rfTransmitterPin")] = this->rfTransmitterPin;
  root[F("rfEchoMessages")] = this->rfEchoMessages;

  {
    DynamicJsonBuffer protoBuffer;
    JsonArray &parsedProtocols = protoBuffer.parseArray(this->rfProtocols);
    JsonArray &protos = root.createNestedArray(F("rfProtocols"));
    for (const auto proto : parsedProtocols) {
      protos.add(proto.as<String>());
    }
  }

  root[F("otaPassword")] = maskSensible(this->otaPassword, sensible);
  root[F("otaUrl")] = this->otaUrl;
  root[F("serialLogLevel")] = this->serialLogLevel;

  if (pretty) {
    root.prettyPrintTo(stream);
  } else {
    root.printTo(stream);
  }
}

void Settings::deserialize(const String &json, const bool fireCallbacks) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject &parsedSettings = jsonBuffer.parseObject(json);

  if (!parsedSettings.success()) {
    Logger.warning.println(F("Config parse failed!"));
    return;
  }

  SettingTypeSet changed;

  changed.set(BASE,
              (setIfPresent(parsedSettings, F("deviceName"), deviceName)));

  changed.set(
      MQTT,
      (setIfPresent(parsedSettings, F("mqttReceiveTopic"), mqttReceiveTopic) ||
       setIfPresent(parsedSettings, F("mqttLogTopic"), mqttLogTopic) ||
       setIfPresent(parsedSettings, F("mqttRawRopic"), mqttRawRopic) ||
       setIfPresent(parsedSettings, F("mqttSendTopic"), mqttSendTopic) ||
       setIfPresent(parsedSettings, F("mqttConfigTopic"), mqttConfigTopic) ||
       setIfPresent(parsedSettings, F("mqttOtaTopic"), mqttOtaTopic) ||
       setIfPresent(parsedSettings, F("mqttBroker"), mqttBroker) ||
       setIfPresent(parsedSettings, F("mqttBrokerPort"), mqttBrokerPort) ||
       setIfPresent(parsedSettings, F("mqttUser"), mqttUser) ||
       setIfPresent(parsedSettings, F("mqttPassword"), mqttPassword) ||
       setIfPresent(parsedSettings, F("mqttRetain"), mqttRetain)));

  changed.set(
      RF_CONFIG,
      (setIfPresent(parsedSettings, F("rfReceiverPin"), rfReceiverPin) ||
       setIfPresent(parsedSettings, F("rfTransmitterPin"), rfTransmitterPin)));

  changed.set(RF_ECHO, (setIfPresent(parsedSettings, F("rfEchoMessages"),
                                     rfEchoMessages)));

  if (parsedSettings.containsKey(F("rfProtocols"))) {
    String buff;
    StringStream stream(buff);
    parsedSettings[F("rfProtocols")].printTo(stream);
    if (buff != rfProtocols) {
      rfProtocols = buff;
      changed.set(RF_PROTOCOL, true);
    }
  }

  changed.set(OTA,
              (setIfPresent(parsedSettings, F("otaPassword"), otaPassword) ||
               setIfPresent(parsedSettings, F("otaUrl"), otaUrl)));

  changed.set(LOGGING, (setIfPresent(parsedSettings, F("serialLogLevel"),
                                     serialLogLevel)));

  if (fireCallbacks) {
    onConfigChange(changed);
  }
}

void Settings::reset() {}
