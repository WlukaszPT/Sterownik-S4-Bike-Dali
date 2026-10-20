# Sterownik-S4-Bike-Dali
## Zawartość

- `S4_BIKE_DALI/S4_BIKE_DALI.ino` – firmware sterownika S-4 Dali (ESP8266). Wersja w `FW_VERSION` i w historii zmian na początku pliku.
- `S4_BIKE_DALI/secrets.example.h` – wzór pliku z danymi konta e-mail.

## Kompilacja

Przed kompilacją skopiuj `secrets.example.h` jako `secrets.h` w folderze szkicu i wpisz dane konta,
z którego sterownik wysyła raporty. Plik `secrets.h` jest w `.gitignore` i nie trafia do repozytorium.

Biblioteki: U8g2, RTClib, Dusk2Dawn, WiFiManager (tzapu), ESP Mail Client. Płytka: ESP8266 (NodeMCU 1.0).

## Wysyłka do eaCloud (od v1.16)

Co 30 s sterownik wysyła `POST /update` (kontrakt v2 eaCloud):

| Pole | Znaczenie |
|---|---|
| `field1` | liczba rowerzystów od poprzedniej wysyłki (wejścia czujników START1/START2, przerwa w sygnale ≥ 0,5 s = nowy rowerzysta) |
| `field2` | czas pracy od uruchomienia [s] |
| `measured_at` | czas wysyłki (unix, UTC) – gdy sterownik ma czas z NTP |
| `message_id` | `s4dali-<chip>-<uruchomienie>-<nr>`; ponowienie po zerwanym połączeniu nie tworzy duplikatu |

Klucz urządzenia idzie w nagłówku `X-SCCLOUDAPIKEY`.

Ustawienia: przytrzymaj **S2 przy starcie**, połącz się z siecią `S-4_Dali_WiFi` i wpisz w sekcji *eaCloud* adres
(np. `http://192.168.1.50:8198/update`) i klucz urządzenia z eaCloud. Pusty adres = wysyłka wyłączona.

- Wysyłka czeka, gdy oczka świecą, trwa alarm, sieć serwisowa albo operacja na magistrali DALI; rowerzyści się nie gubią.
- Brak odpowiedzi serwera: ta sama wiadomość jest ponawiana, przerwa rośnie 30 s → 60 s → … → 5 min.
- Odpowiedź `0` (zły klucz lub dane): bez ponawiania, wpis w dzienniku.
- Stan wysyłki: dziennik i stopka strony serwisowej, raport e-mail.
- Na razie tylko `http://` (serwer w sieci lokalnej). HTTPS do serwera TEST/PROD – kolejny krok.
