# MyHome Assist ESP32 🏠

Um projeto de automação residencial e monitoramento inteligente usando o microcontrolador **ESP32**. Este sistema atua como uma central de sensores e alarme, oferecendo uma interface web simples e uma **API REST completa** para integração.

## 🌟 Funcionalidades

- **Monitoramento de Ambiente**: Leitura em tempo real de Temperatura, Umidade e Luminosidade.
- **Sistema de Alarme Integrado**: Controle remoto de um alarme com campainha (Buzzer) que toca a _Marcha Imperial_!
- **Modo Ponto de Acesso (AP)**: Quando não configurado, o ESP32 cria uma rede Wi-Fi própria (`ESP32-SETUP-ARTHUR`) para que as credenciais da rede principal sejam cadastradas.
- **Resiliência de Conexão**: As credenciais Wi-Fi são salvas permanentemente na memória (usando `Preferences`).
- **Reset Físico**: Pressionar o botão BOOT por 3 segundos limpa as configurações de rede e reinicia o aparelho.
- **mDNS Habilitado**: Acesse o dispositivo localmente através de `http://alarme.local` sem precisar decorar o IP.
- **Coleções de API Prontas**: A pasta `API-Collections-MYHOMEASSIST` contém todos os endpoints no formato Bruno (`.yml`) para testes fáceis das rotas.

## 🛠️ Hardware e Pinagem

O projeto foi configurado com a seguinte disposição de hardware (facilmente alterável no `main.cpp`):

| Componente | Pino do ESP32 | Função |
| :--- | :--- | :--- |
| **LED Embutido** | `GPIO 2` | Indicação visual do estado do alarme e reset |
| **Buzzer** | `GPIO 18` | Alarme sonoro (Canal PWM 0) |
| **Sensor de Luz (LDR)**| `GPIO 39` | Medição de luminosidade ambiente (ADC) |
| **Termistor** | `GPIO 36` | Medição analógica de precisão para temperatura |
| **Sensor DHT11** | `GPIO 13` | Leitura de umidade (e temperatura secundária) |
| **Botão (BOOT)** | `GPIO 0` | Usado para Hard-Reset do Wi-Fi |

## 📡 Endpoints da API REST

A comunicação com o ESP32 é feita nativamente por endpoints JSON simples.

- `GET /` - Retorna a página Web UI básica com controles do alarme e reset.
- `GET /status` - Retorna o MAC Address e se o Wi-Fi já está configurado.
- `POST /wifi` - Configura as credenciais da rede Wi-Fi. (Espera um JSON com `ssid` e `password`).
- `GET /temperature` - Retorna a temperatura atual em °C.
- `GET /humidity` - Retorna a umidade atual em %.
- `GET /luminosity` - Retorna a claridade/luminosidade atual em %.
- `GET /H` - **Ativa** o alarme.
- `GET /L` - **Desativa** o alarme.
- `GET /info` - Retorna as informações de rede do ESP32 (IP, Hostname, Status).
- `GET /reset` - Limpa os dados de Wi-Fi e reinicia a placa remotamente.

> **Dica**: Utilize o client [Bruno](https://www.usebruno.com/) para importar a pasta `API-Collections-MYHOMEASSIST` e testar todas as requisições rapidamente!

## 🚀 Como Compilar e Rodar

1. Clone o repositório em seu computador.
2. Abra a pasta do projeto no **VS Code** com a extensão do **PlatformIO** instalada.
3. Certifique-se de que as bibliotecas necessárias (`ArduinoJson`, `DHT sensor library for ESPx`, etc.) estão declaradas no seu `platformio.ini`.
4. Conecte seu ESP32 via USB.
5. Clique em **Build** e depois em **Upload**.
6. (Opcional) Acompanhe o **Serial Monitor** em `115200` baud rate para ver o IP ou o status do AP criado.

---
Desenvolvido com ☕ e C++ para automação residencial.
