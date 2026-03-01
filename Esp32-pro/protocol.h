/*
 * protocol.h - Protocole Nintendo Switch
 * ESP32 Pro Controller
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <Arduino.h>

// ========== FONCTIONS ==========

void packButtons(uint8_t* dest);
void handleSubcommand(uint8_t cmd, const uint8_t* data, size_t len);
void handleOutputReport(const uint8_t* data, size_t len);

void sendAck(uint8_t subcommand);
void sendDeviceInfo();
void sendSPIRead(const uint8_t* data, size_t len);
void sendInputReport();

#endif