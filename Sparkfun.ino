
// my Chip IDs to give it a cleartype name
#define WA1 14117293
#define WA2 12612352


bool sendSparkfun(byte sparkfunType) {
  
// Use Sparkfun testing stream
const char* host = "data.sparkfun.com";
const char* streamId   = "q5Jpbl9jmpFYgLMr8zQv";
const char* privateKey = "BVBnz5ywgnSJpz68ek7r";
  
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

  String url = "/input/";
  url += streamId;
  url += "?private_key=";
  url += privateKey;
  url += "&battery=";
  url += float(ESP.getVcc() * VCC_ADJ); // Could be any string
  
  Serial.println();
  Serial.println("sparkfun: ");
  Serial.println();

  Serial.print("Request URL: "); Serial.println(url);

  client.print(String("GET ") + url +
               " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

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
