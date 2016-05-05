
// Use Sparkfun testing stream



// my Chip IDs to give it a cleartype name
#define WA1 14117293
#define WA2 12612352


bool sendubidots(byte ubidotsType) {
  const char* host = "things.ubidots.com";
  const char* streamId   = ubidotsStreamId;
  const char* token = ubidotsToken;
  Serial.print("Connecting to "); Serial.print(host);

  WiFiClient client;
  int retries = 5;
  while (!client.connect(host, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if (!client.connected()) {
    Serial.println("Failed to connect, going back to sleep");
    return false;
  }

  String url = "/api/v1.6/variables/";
  url += streamId;


  Serial.println();
  Serial.println("UBIDOTS: ");
  Serial.println();

  String value = String(float(ESP.getVcc() * VCC_ADJ));
  String valueStr =   "{\"value\":" + value + '}';

  String urlLoad = String("POST ") + url +
                   "/values HTTP/1.1\r\n" +
                   "X-Auth-Token: " + token + "\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n" +
                   "Content-Type: application/json\r\n" +
                   "Content-Length: "  + valueStr.length() + "\r\n\r\n" +
                   valueStr + "\r\n\r\n";

  Serial.print(urlLoad);
  client.print(urlLoad);

  int timeout = 5 * 10; // 5 seconds
  while (!client.available() && (timeout-- > 0)) {
    delay(100);
  }

  if (!client.available()) {
    Serial.println("No response, going back to sleep");
    return false;
  }
  Serial.println(F("disconnected"));
  return true;
}
