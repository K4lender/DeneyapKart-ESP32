/*
 *  KameraYuzTanima örneği,
 *  Bu örnek WiFi ağı ve şifresini girdikten sonra ağa bağlanacaktır. 
 *  WiFi ağına bağlandıktan sonra seri port ekranından görüntünün yayınlanacağı IP adresi yazılacaktır.
 * 
 *  Bu örnek kamera konnektörü dahili olan Deneyap Geliştirme Kartlarını desteklemektedir.  
*/
// ---------->>>>>>>>>> YUKLEME YAPILAMDAN DIKKAT EDILMESI GEREKEN HUSUS <<<<<<<<<<----------
// "Araclar->Partition Scheme->Huge APP" secilmeli //
// "Tools->Partition Scheme->Huge APP" secilmeli //

#include "WiFi.h"
#include "DeneyapEsp32cam.h"

const char* ssid = "EYUP DENEYAP";      // Baglanti kurulacak Wi-Fi agi adi
const char* password = "MTH#122016?!.";  // Baglanti kurulacak Wi-Fi agi sifresi

void startCameraServer();
esp32cam::CameraClass cam;

void setup() {
  Serial.begin(115200);  // Hata ayiklamak icin seri port ekran baslatildi
  Serial.setDebugOutput(true);
  Serial.println();

  cam.cameraInit();  // Kamera konfigurasyonu yapildi

  WiFi.begin(ssid, password);  // Wi-Fi agina baglaniliyor

  while (WiFi.status() != WL_CONNECTED) {  // Baglanti saglanan kadar bekleniyor
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Wi-Fi agina baglanildi ");

  startCameraServer();  // Kamera server baslatildi

  Serial.print("Kamera hazir! Baglanmak icin 'http://");  // Baglanti saglandi
  Serial.print(WiFi.localIP());                           // Goruntunun yayinlanacagi IP adresi seri port ekranına yaziliyor
  Serial.println("' adresini kullaniniz");
}

void loop() {
  delay(1000);
}
